#include "scanner.hpp"
#include <iostream>
#include <string>

std::string PatternToken::toString() const {
    if (isWildcard)
        return "?";
    if (mask != 0xFF)
        return std::format("{:02X}&{:02X}", byte, mask);
    return std::format("{:02X}", byte);
}

bool PatternToken::matches(uint8_t b) const {
    return isWildcard || ((byte & mask) == (b & mask));
}

bool PatternToken::operator==(uint8_t b) const {
    return matches(b);
}

bool PatternToken::operator!=(uint8_t b) const {
    return !matches(b);
}

bool PatternToken::operator==(const PatternToken &other) const {
    return isWildcard == other.isWildcard && byte == other.byte && mask == other.mask;
}

bool PatternToken::operator!=(const PatternToken &other) const {
    return !(*this == other);
}

PatternToken PatternToken::wildcard() {
    return PatternToken(true, 0);
}

PatternToken PatternToken::fromByte(uint8_t byte) {
    return PatternToken(false, byte);
}

PatternToken PatternToken::fromByteMask(uint8_t byte, uint8_t mask) {
    // if the mask is 0xFF, we can just use fromByte
    if (mask == 0xFF)
        return PatternToken(false, byte);

    // if the mask is 0x00, this is a wildcard
    if (mask == 0)
        return PatternToken(true, 0);

    return PatternToken(false, byte, mask);
}

void swallowWhitespace(std::string_view pattern, size_t &i) {
    while (i < pattern.size() && pattern[i] == ' ') { i++; }
}

char peek(std::string_view pattern, size_t i) {
    return i < pattern.size() ? pattern[i] : '\0';
}

std::vector<PatternToken> PatternToken::fromString(std::string_view pattern) {
    std::vector<PatternToken> tokens;
    for (size_t i = 0; i < pattern.size(); i++) {
        swallowWhitespace(pattern, i);

        if (pattern[i] == '?') {
            tokens.push_back(PatternToken::wildcard());
        } else {
            uint8_t byte = std::stoi(std::string(pattern.substr(i, 2)), nullptr, 16);
            // check if the next character is an ampersand
            if (peek(pattern, i + 2) == '&') {
                uint8_t mask = std::stoi(std::string(pattern.substr(i + 3, 2)), nullptr, 16);
                tokens.push_back(PatternToken::fromByteMask(byte, mask));
                i += 5;
            } else {
                tokens.push_back(PatternToken::fromByte(byte));
                i++;
            }
        }
    }

    // make sure we got all the tokens
    // std::cout << std::format("o: {}\nn: {}\n", pattern, fromPatternTokens(tokens));

    return tokens;
}

bool Scanner::find(const std::vector<PatternToken> &tokens, std::vector<uintptr_t> &results) const {
    for (size_t i = 0; i < binary.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < tokens.size(); j++) {
            if (i + j >= binary.size() || tokens[j] != binary[i + j]) {
                found = false;
                break;
            }
        }

        if (found) {
            results.push_back(i + baseAddress);
        }
    }

    return !results.empty();
}

bool Scanner::find(std::string_view pattern, std::vector<uintptr_t> &results) const {
    return find(PatternToken::fromString(pattern), results);
}

bool Scanner::find(std::string_view pattern, uintptr_t &result) const {
    std::vector<uintptr_t> results;
    if (!find(pattern, results))
        return false;

    if (results.empty())
        return false;

    result = results[0];
    return true;
}

std::string Scanner::generateUniquePattern(uintptr_t address, size_t maxLength) const {
    // Add bytes to the pattern until we reach the maximum length or only one address is found
    std::vector<PatternToken> pattern;
    address += baseAddress;
    bool found = false;
    for (size_t i = 0; i < maxLength; i++) {
        pattern.push_back(PatternToken::fromByte(binary[address + i]));
        std::vector<uintptr_t> results;
        if (find(pattern, results) && results.size() == 1) {
            found = true;
            break;
        }
    }

    if (!found) return "";

    std::string patternString;
    for (const auto& token : pattern) {
        patternString += token.toString();
        patternString += " ";
    }

    return patternString;
}

std::vector<uint8_t> Scanner::getSubArray(uintptr_t address, size_t length) const {
    std::vector<uint8_t> subArray;
    for (size_t i = address + baseAddress; i < address + baseAddress + length; i++) {
        subArray.push_back(binary[i]);
    }
    return subArray;
}
