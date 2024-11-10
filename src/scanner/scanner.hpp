#pragma once
#include <vector>
#include <cstdint>
#include <utility>
#include <string_view>
#include <format>
#include <span>

struct PatternToken {
    bool isWildcard;
    uint8_t byte;
    uint8_t mask;

    explicit PatternToken(bool isWildcard, uint8_t byte, uint8_t mask = 0xFF)
        : isWildcard(isWildcard), byte(byte), mask(mask) {}

    [[nodiscard]] bool matches(uint8_t b) const;

    [[nodiscard]] bool operator==(uint8_t b) const;
    [[nodiscard]] bool operator!=(uint8_t b) const;

    [[nodiscard]] bool operator==(const PatternToken& other) const;
    [[nodiscard]] bool operator!=(const PatternToken& other) const;

    [[nodiscard]] std::string toString() const;

    static PatternToken wildcard();
    static PatternToken fromByte(uint8_t byte);
    static PatternToken fromByteMask(uint8_t byte, uint8_t mask);
    static std::vector<PatternToken> fromString(std::string_view pattern);

    static std::string fromPatternTokens(const std::vector<PatternToken>& tokens) {
        std::string result;

        // strip trailing wildcards
        std::span tokensSpan(tokens);
        while (!tokensSpan.empty() && tokensSpan.back() == 0)
            tokensSpan = tokensSpan.subspan(0, tokensSpan.size() - 1);

        for (const auto& token : tokensSpan) {
            result += token.toString();
            result += " ";
        }

        // Remove trailing space
        if (!result.empty())
            result.pop_back();

        return result;
    }
};

class Scanner {
public:
    Scanner(std::vector<uint8_t> binary, intptr_t baseAddress)
        : binary(std::move(binary)), baseAddress(baseAddress) {}

    bool find(const std::vector<PatternToken> &tokens, std::vector<uintptr_t>& results) const;
    bool find(std::string_view pattern, std::vector<uintptr_t>& results) const;
    bool find(std::string_view pattern, uintptr_t& result) const;

    [[nodiscard]] std::vector<uint8_t> getSubArray(uintptr_t address, size_t length) const;

    [[nodiscard]] std::string generateUniquePattern(uintptr_t address, size_t maxLength) const;

private:
    std::vector<uint8_t> binary;
    intptr_t baseAddress;
};