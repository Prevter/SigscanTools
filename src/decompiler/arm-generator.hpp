#pragma once
#include <capstone/capstone.h>

#include "../scanner/scanner.hpp"

/// Handles the generation of ARM64 patterns
class ArmGenerator {
public:
    [[nodiscard]] static std::vector<PatternToken> getPattern(std::vector<uint8_t> const& bytes, std::string_view text, cs_arm64 const& data);

private:
#define DEFINE_OP(name) static std::vector<PatternToken> name(std::vector<uint8_t> const& bytes, cs_arm64 const& data)

    // TODO: Implement all ARM64 instructions (:sweat:)

    DEFINE_OP(add);
    DEFINE_OP(adrp);
    DEFINE_OP(bl);
    DEFINE_OP(cbz);
    DEFINE_OP(cmp);
    DEFINE_OP(ldp);
    DEFINE_OP(ldr);
    DEFINE_OP(mov);
    DEFINE_OP(ret);
    DEFINE_OP(stp);
    DEFINE_OP(str);
    DEFINE_OP(sub);

#undef DEFINE_OP
};