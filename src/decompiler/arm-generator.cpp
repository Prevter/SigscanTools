#include "arm-generator.hpp"

#include <array>
#include <iostream>
#include <unordered_map>

std::vector<PatternToken> ArmGenerator::getPattern(std::vector<uint8_t> const &bytes, std::string_view text, cs_arm64 const &data) {
    if (bytes.size() != 4) goto IDK;

    static std::unordered_map<std::string_view, std::vector<PatternToken> (*)(std::vector<uint8_t> const &, cs_arm64 const &)> const instructions = {
        {"add",  add},
        {"adrp", adrp},
        {"b",    b},
        {"bl",   bl},
        {"blr",  blr},
        {"cbz",  cbz},
        {"cmp",  cmp},
        {"ldp",  ldp},
        {"ldr",  ldr},
        {"mov",  mov},
        {"ret",  ret},
        {"stp",  stp},
        {"str",  str},
        {"strb", strb},
        {"sub",  sub},
    };

    if (const auto it = instructions.find(text); it != instructions.end()) {
        return std::move(it->second(bytes, data));
    }

    // we don't know what this is, so just wildcard it
    IDK:
    // std::cout << std::format("Unknown instruction: {}\n", text);
    std::vector<PatternToken> pattern;
    pattern.reserve(bytes.size());
    for (size_t i = 0; i < bytes.size(); i++) {
        pattern.push_back(PatternToken::wildcard());
    }
    return pattern;
}

/// NOTE:
/// ARM is Little Endian, so byte masks are reversed

std::vector<PatternToken> ArmGenerator::add(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    // ADD(ext)   0b11111111 0b11100000 0b11111100 0b00000000
    // ADD(imm)   0b11111111 0b11111111 0b11111100 0b00000000
    // ADD(shift) 0b11111111 0b11100000 0b11111100 0b00000000
    return {
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[1], 0b11111100),
        PatternToken::fromByteMask(bytes[2], 0b11100000),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::adrp(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[3], 0b10011111),
    };
}

std::vector<PatternToken> ArmGenerator::b(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[3], 0b11111100),
    };
}

std::vector<PatternToken> ArmGenerator::bl(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[3], 0b11111100),
    };
}

std::vector<PatternToken> ArmGenerator::blr(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::fromByteMask(bytes[0], 0b00011111),
        PatternToken::fromByteMask(bytes[1], 0b11111100),
        PatternToken::fromByte(bytes[2]),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::cbz(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::cmp(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::fromByteMask(bytes[0], 0b00011111),
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[2], 0b11100000),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::ldp(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[1], 0b10000000),
        PatternToken::fromByte(bytes[2]),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::ldr(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    // LDR(imm)     0b11111111 0b11100000 0b00001100 0b00000000
    // LDR(literal) 0b11111111 0b00000000 0b00000000 0b00000000
    // LDR(reg)     0b11111111 0b11100000 0b11111100 0b00000000
    return {
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::mov(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    // MOV(bitmask)        0b11111111 0b11111111 0b11111111 0b11100000
    // MOV(int. wide imm.) 0b11111111 0b11111111 0b11111111 0b11100000
    // MOV(register)       0b11111111 0b11100000 0b11111111 0b11100000
    // MOV(to/from SP)     0b11111111 0b11111111 0b11111100 0b00000000
    // MOV(wide imm.)      0b11111111 0b11111111 0b11111111 0b11100000
    return {
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[1], 0b11111100),
        PatternToken::fromByteMask(bytes[2], 0b11100000),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::ret(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::fromByteMask(bytes[0], 0b00011111),
        PatternToken::fromByteMask(bytes[1], 0b11111100),
        PatternToken::fromByte(bytes[2]),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::stp(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[1], 0b10000000),
        PatternToken::fromByte(bytes[2]),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::str(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    return {
        PatternToken::wildcard(),
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[2], 0b11000000),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::strb(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    // STRB (imm) 0b11111111 0b11111111 0b11111100 0b00000000
    // STRB (reg) 0b11111111 0b11100000 0b11111100 0b00000000
    return {
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[1], 0b11111100),
        PatternToken::fromByteMask(bytes[2], 0b11100000),
        PatternToken::fromByte(bytes[3]),
    };
}

std::vector<PatternToken> ArmGenerator::sub(std::vector<uint8_t> const &bytes, cs_arm64 const &data) {
    // SUB(ext)   0b11111111 0b11100000 0b11111100 0b00000000
    // SUB(imm)   0b11111111 0b11111111 0b11111100 0b00000000
    // SUB(shift) 0b11111111 0b11100000 0b11111100 0b00000000
    return {
        PatternToken::wildcard(),
        PatternToken::fromByteMask(bytes[1], 0b11111100),
        PatternToken::fromByteMask(bytes[2], 0b11100000),
        PatternToken::fromByte(bytes[3]),
    };
}

