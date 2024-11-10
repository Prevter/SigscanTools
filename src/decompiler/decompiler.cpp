#include "decompiler.hpp"
#include <iostream>
#include "arm-generator.hpp"

inline std::string to_string(const std::vector<uint8_t>& bytes) {
    std::string result;
    for (auto byte : bytes) {
        result += std::format("{:02X} ", byte);
    }
    if (!result.empty()) {
        result.pop_back();
    }
    return result;
}

std::vector<PatternToken> Opcode::getSafePattern() const {
    if (isCapstone) return ArmGenerator::getPattern(bytes, text, capstone.detail);

    std::vector<PatternToken> pattern;
    pattern.reserve(bytes.size());

    auto copyBytes = [&](size_t offset, size_t count, bool wildcard = false) {
        for (size_t i = 0; i < count; i++) {
            if (wildcard) {
                pattern.push_back(PatternToken::wildcard());
            } else {
                pattern.push_back(PatternToken::fromByte(bytes[offset + i]));
            }
        }
    };

    for (int i = 0; i < zydis.segments.count; i++) {
        auto segment = zydis.segments.segments[i];
        switch (segment.type) {
            case ZYDIS_INSTR_SEGMENT_DISPLACEMENT:
                copyBytes(segment.offset, segment.size, true);
                break;
            default:
                copyBytes(segment.offset, segment.size);
                break;
        }
    }

    return pattern;
}

void Decompiler::decompile(uintptr_t address, size_t size, std::vector<Opcode> &opcodes) const {
    ZydisMachineMode machineMode;
    switch (arch) {
        case Arch::x86:
            machineMode = ZYDIS_MACHINE_MODE_LONG_COMPAT_32;
            break;
        case Arch::x86_64:
            machineMode = ZYDIS_MACHINE_MODE_LONG_64;
            break;
        default:
            // use capstone for arm
            return decompileCapstone(address, size, opcodes);
    }

    auto data = scanner.getSubArray(address, size);

    ZyanUSize offset = 0;
    ZydisDisassembledInstruction ins;
    while (ZYAN_SUCCESS(ZydisDisassembleIntel(
            /* machine_mode:    */ machineMode,
            /* runtime_address: */ address + offset,
            /* buffer:          */ data.data() + offset,
            /* length:          */ size - offset,
            /* instruction:     */ &ins
    ))) {
        auto text = ins.text;
        auto len = ins.info.length;

        // return early if hit an int3
        if (ins.info.mnemonic == ZYDIS_MNEMONIC_INT3) {
            return;
        }

        ZydisInstructionSegments segments;
        ZydisGetInstructionSegments(&ins.info, &segments);

        Opcode opcode;
        opcode.address = address + offset;
        opcode.length = len;
        opcode.zydis.segments = segments;
        opcode.text = std::string(text);
        opcode.bytes = std::vector<uint8_t>(data.begin() + offset, data.begin() + offset + len);
        opcode.zydis.mnemonic = ins.info.mnemonic;
        opcodes.push_back(opcode);

        offset += len;
    }
}

void Decompiler::decompileCapstone(uintptr_t address, size_t size, std::vector<Opcode> &opcodes) const {
    csh handle;
    cs_arch architecture;
    cs_mode mode;
    switch (arch) {
        case Arch::armv8:
            mode = CS_MODE_ARM;
            architecture = CS_ARCH_ARM64;
            break;
        default:
            return;
    }

    if (cs_open(architecture, mode, &handle) != CS_ERR_OK) {
        std::cerr << "Failed to initialize capstone" << std::endl;
        return;
    }

    // std::cout << std::format("Decompiling capstone at {:X} size {:X}\n", address, size);

    auto data = scanner.getSubArray(address, size);
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    cs_insn *insn;
    size_t count = cs_disasm(handle, data.data(), data.size(), address, 0, &insn);
    if (count > 0) {
        for (size_t i = 0; i < count; i++) {
            auto &ins = insn[i];
            auto text = ins.mnemonic;
            auto len = ins.size;

            Opcode opcode;
            opcode.isCapstone = true;
            opcode.address = ins.address;
            opcode.length = len;
            opcode.text = text;
            opcode.bytes = std::vector<uint8_t>(ins.bytes, ins.bytes + len);

            opcode.capstone.detail = ins.detail->arm64;

            // std::cout << std::format("Decompiled: {:X} {}\n", opcode.address, opcode.text);

            opcodes.push_back(opcode);
        }

        cs_free(insn, count);
    }

    cs_close(&handle);
}
