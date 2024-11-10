#include <iostream>
#include <fstream>
#include <optional>
#include <string>
#include "scanner/scanner.hpp"

std::vector<std::string> split(std::string_view str, char i);

int main(int argc, char** argv) {
    // importer.exe <binary-path> <> <bindings-origin-path> <bindings-target-path> <file-offset>
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <binary-path> <patterns> <output> <file-offset>" << std::endl;
        std::cerr << "Example: " << argv[0] << " GeometryDash2203.exe output2204.csv found2203.csv -0xC00" << std::endl;
        return 1;
    }

    std::string binaryPath = argv[1];
    std::string patternsPath = argv[2];
    std::string outputPath = argv[3];
    int64_t fileOffset = std::stoll(argv[4], nullptr, 16);

    std::cout << "Binary path: " << binaryPath << std::endl;
    std::cout << "Patterns path: " << patternsPath << std::endl;
    std::cout << "Output path: " << outputPath << std::endl;
    std::cout << "File offset: " << fileOffset << std::endl;

    std::ifstream binaryFile(binaryPath, std::ios::binary);
    if (!binaryFile.is_open()) {
        std::cerr << "Failed to open binary file: " << binaryPath << std::endl;
        return 1;
    }

    Scanner scanner(
        (std::vector<uint8_t>(std::istreambuf_iterator<char>(binaryFile), std::istreambuf_iterator<char>())),
        fileOffset
    );

    std::ifstream patternsFile(patternsPath);
    if (!patternsFile.is_open()) {
        std::cerr << "Failed to open patterns file: " << patternsPath << std::endl;
        return 1;
    }

    std::ofstream outputFile(outputPath);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(patternsFile, line)) {
        auto parts = split(line, ',');
        if (parts.size() != 3) {
            std::cerr << "Invalid line: " << line << std::endl;
            continue;
        }

        uintptr_t offset = std::stoll(parts[0], nullptr, 16);
        const auto& name = parts[1];
        const auto& pattern = parts[2];

        auto results = std::vector<uintptr_t>();
        if (scanner.find(pattern, results)) {
            // filter out results that are too far away from original offset
            constexpr auto maxDistance = 0x50000;
            std::erase_if(results, [offset, maxDistance](uintptr_t result) {
                if (result < offset)
                    return offset - result > maxDistance;
                return result - offset > maxDistance;
            });

            if (results.size() > 1) {
                std::cerr << std::format("Multiple results found: {:X} {}\n", offset, name);
                continue;
            }

            for (auto result : results) {
                // output:
                // offset in original binary,name,new offset in new binary
                outputFile << std::format("{:X},{},{:X}\n", offset, name, result);
                std::cout << std::format("Found: {:X} {} at {:X}\n", offset, name, result);
            }
        } else {
            std::cerr << std::format("Pattern not found: {:X} {}\n", offset, name);
        }
    }
}

std::vector<std::string> split(std::string_view str, char i) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = str.find(i);
    while (end != std::string::npos) {
        parts.emplace_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(i, start);
    }
    parts.emplace_back(str.substr(start, end));
    return parts;
}
