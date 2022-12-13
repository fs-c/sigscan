#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

class Signature {
    uintptr_t offset;
    std::vector<std::pair<uint8_t, bool>> pattern;

    // converts a pattern like "AB CD EF ? ? FF" to [ [171, false], ..., [0, true], ... ]
    // where the second pair member signifies whether the byte is a wildcard
    void parse_string_pattern(std::string_view string_pattern) {
        const auto raw_pattern = string_pattern.data();

        for (auto cur = raw_pattern; *cur; cur++) {
            if (*cur == '?') {
                pattern.emplace_back(0, true);
            } else if (*cur == ' ') {
                continue;
            } else {
                const auto value = strtol(cur, const_cast<char **>(&cur), 16);

                pattern.emplace_back(static_cast<uint8_t>(value), false);
            }
        }
    }

public:
    Signature(std::string_view pattern, uintptr_t offset) : offset(offset) {
        parse_string_pattern(pattern);
    }

    uintptr_t scan(const std::vector<uint8_t> &buffer) const {
        for (int i = 0; i < buffer.size() - pattern.size(); i++) {
            bool hit = true;
            for (int j = 0; j < pattern.size(); j++) {
                if (buffer.at(i + j) != pattern.at(j).first) {
                    hit = false;
                    break;
                }
            }

            if (hit) {
                return reinterpret_cast<uintptr_t>(buffer.data()) + i;
            }
        }

        return 0;

//        const auto comparator = [](auto byte, auto pair) {
//            return byte == pair.first;
//        };
//
//        const auto result = std::search(buffer.begin(), buffer.end(),
//                pattern.begin(), pattern.end(), comparator);
//
//        return result == buffer.end() ? 0 : (result - buffer.begin() + offset);
    }
};
