#include <iostream>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#include "scanner/scanner.hpp"
#include "decompiler/decompiler.hpp"

struct FunctionSignature {
    std::string name;
    std::string signature;
};

std::optional<FunctionSignature> findSignature(std::string_view name, uintptr_t address, size_t size, const Scanner& scanner, const Decompiler& decompiler) {
    std::vector<Opcode> opcodes;
    decompiler.decompile(address, size, opcodes);

    std::vector<PatternToken> pattern;
    for (const auto& opcode : opcodes) {
        // add opcode to pattern and check if it matches any of the signatures
        auto opcodePattern = opcode.getSafePattern();
        pattern.insert(pattern.end(), opcodePattern.begin(), opcodePattern.end());

        // scan for the pattern
        std::vector<uintptr_t> results;
        if (!scanner.find(pattern, results))
            return std::nullopt;

        // check if only one result was found
        if (results.size() != 1)
            continue;

        // construct the function signature
        FunctionSignature signature;
        signature.name = std::string(name);
        signature.signature = PatternToken::fromPatternTokens(pattern);
        return signature;
    }

    return std::nullopt;
}

struct SearchTask {
    std::string name;
    uintptr_t address;
    size_t size;

    SearchTask(std::string name, uintptr_t address, size_t size)
        : name(std::move(name)), address(address), size(size) {}

    bool finished = false;
    std::optional<FunctionSignature> signature = std::nullopt;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency()) : m_threads(threads) {}

    void addTask(std::function<void()>&& task) {
        m_tasks.push_back(std::move(task));
    }

    void runAllTasks() {
        // create threads
        for (size_t i = 0; i < m_threads; i++) {
            m_workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::lock_guard lock(m_tasksMutex);
                        if (m_tasks.empty())
                            break;

                        task = std::move(m_tasks.back());
                        m_tasks.pop_back();
                    }

                    task();
                }
            });
        }

        // join threads
        for (auto& worker : m_workers) {
            worker.join();
        }
    }

private:
    size_t m_threads;
    std::vector<std::thread> m_workers;
    std::vector<std::function<void()>> m_tasks;
    std::mutex m_tasksMutex;
};

int main(int argc, char* argv[]) {
    if (argc != 5 && argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <binary-path> <bindings-path> <output> <file-offset> [arch=x64]" << std::endl;
        std::cerr << "Example: " << argv[0] << " GeometryDash.exe funcs.csv output.txt -0xC00 x32" << std::endl;
        return 1;
    }

    std::string binaryPath = argv[1];
    std::string bindingsPath = argv[2];
    std::string outputPath = argv[3];
    int64_t fileOffset = std::stoll(argv[4], nullptr, 16);
    std::string arch = argc == 6 ? argv[5] : "x64";

    std::cout << "Binary path: " << binaryPath << std::endl;
    std::cout << "Bindings path: " << bindingsPath << std::endl;
    std::cout << "Output path: " << outputPath << std::endl;
    std::cout << "File offset: " << fileOffset << std::endl;

    std::ifstream binaryFile(binaryPath, std::ios::binary);
    if (!binaryFile.is_open()) {
        std::cerr << "Failed to open binary file: " << binaryPath << std::endl;
        return 1;
    }

    Scanner scanner(
        (std::vector<uint8_t>(std::istreambuf_iterator(binaryFile), std::istreambuf_iterator<char>())),
        fileOffset
    );

    Decompiler::Arch decompilerArch;
    if (arch == "x32" || arch == "x86") {
        decompilerArch = Decompiler::Arch::x86;
    } else if (arch == "x64" || arch == "x86_64") {
        decompilerArch = Decompiler::Arch::x86_64;
    } else if (arch == "armv7" || arch == "arm32") {
        decompilerArch = Decompiler::Arch::armv7;
    } else if (arch == "armv8" || arch == "arm64") {
        decompilerArch = Decompiler::Arch::armv8;
    } else {
        std::cerr << "Invalid architecture: " << arch << std::endl;
        return 1;
    }

    Decompiler decompiler(scanner, decompilerArch);

    std::ifstream bindingsFile(bindingsPath);
    if (!bindingsFile.is_open()) {
        std::cerr << "Failed to open bindings file: " << bindingsPath << std::endl;
        return 1;
    }

    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        return 1;
    }

    std::vector<SearchTask> tasks;

    // CSV format: <name>,<address>,<size>
    std::string line;
    std::getline(bindingsFile, line); // skip header
    while (std::getline(bindingsFile, line)) {
        auto commaPos = line.find(',');
        if (commaPos == std::string::npos) {
            std::cerr << "Invalid line in bindings file: " << line << std::endl;
            return 1;
        }

        std::string name = line.substr(0, commaPos);
        uintptr_t address = std::stoul(line.substr(commaPos + 1, line.find(',', commaPos + 1) - commaPos - 1), nullptr);
        size_t size = std::stoul(line.substr(line.find(',', commaPos + 1) + 1), nullptr);

        tasks.emplace_back(name, address, size);
    }

    std::mutex outputMutex, consoleMutex;
    auto writeToFile = [&](const std::string& text) {
        std::lock_guard lock(outputMutex);
        outputFile << text;
    };

    auto writeToConsole = [&](const std::string& text, bool error = false) {
        std::lock_guard lock(consoleMutex);
        if (error) {
            std::cerr << text;
        } else {
            std::cout << text;
        }
    };

    std::atomic<int> count = 0, total = 0, failed = 0;

    ThreadPool pool;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (auto& task : tasks) {
        pool.addTask([&, task = std::ref(task)] {
            auto signature = findSignature(task.get().name, task.get().address, task.get().size, scanner, decompiler);
            if (signature.has_value()) {
                auto o = std::format("0x{:X},{},{}\n", task.get().address, signature->name, signature->signature);
                writeToFile(o);
                // writeToConsole(o);
                ++count;
            } else {
                // writeToConsole(std::format("{}: not found (size: {})\n", task.get().name, task.get().size), true);
                ++failed;
            }

            ++total;
            task.get().finished = true;
        });
    }

    pool.runAllTasks();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    int count_ = count;
    int total_ = total;
    int failed_ = failed;
    std::cout << std::format("Found {}/{} signatures ({} failed)\n", count_, total_, failed_);
    std::cout << std::format("Time taken: {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    return 0;
}