#pragma once
#include <string>
#include <vector>
#include <Zydis/Zydis.h>
#include <capstone/capstone.h>
#include "../scanner/scanner.hpp"

struct Opcode {
    uintptr_t address;
    uint8_t length;

    union {
        struct ZydisInfo {
            ZydisInstructionSegments segments;
            ZydisMnemonic mnemonic;
        } zydis;

        struct CapstoneInfo {
            cs_arm64 detail;
        } capstone;
    };
    bool isCapstone = false;

    std::string text;
    std::string description;

    std::vector<uint8_t> bytes;

    [[nodiscard]] std::vector<PatternToken> getSafePattern() const;
};

class Decompiler {
public:
    enum class Arch {
        x86,
        x86_64,
        armv7,
        armv8
    };

    Decompiler(Scanner& scanner, Arch arch) : scanner(scanner), arch(arch) {}

    void decompile(uintptr_t address, size_t size, std::vector<Opcode>& opcodes) const;
    void decompileCapstone(uintptr_t address, size_t size, std::vector<Opcode>& opcodes) const;
private:
    Scanner& scanner;
    Arch arch;
};