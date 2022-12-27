#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <array>
#include <algorithm>

class Signature {
    uintptr_t offset;
    std::vector<std::pair<uint8_t, bool>> pattern;

    // converts a pattern like "AB CD EF ? ? FF" to [ [171, false], ..., [0, true], ... ]
    // where the second pair member signifies whether the byte is a wildcard
    constexpr void parse_string_pattern(const std::string &string_pattern) {
        for (auto cur = string_pattern.data(); *cur; cur++) {
            if (*cur == '?') {
                pattern.emplace_back(0, true);
            } else if (*cur == ' ') {
                continue;
            } else {
                const auto value = strtol(cur, const_cast<char **>(&cur), 16);

                pattern.emplace_back(static_cast<uint8_t>(value), false);

                if (!*cur) {
                    break;
                }
            }
        }
    }

public:
    Signature(const std::string &pattern, uintptr_t offset) : offset(offset) {
        parse_string_pattern(pattern);
    }

    uintptr_t scan(const std::vector<uint8_t> &buffer) const {
        const auto comparator = [](auto byte, auto pair) {
            return pair.second || byte == pair.first;
        };

        const auto result = std::search(buffer.begin(), buffer.end(),
            pattern.begin(), pattern.end(), comparator);

        return result == buffer.end() ? 0 : (result - buffer.begin() + offset);
    }
};
