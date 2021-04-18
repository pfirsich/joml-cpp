#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include "joml.hpp"

#define CONTEXT getContextString(str, getPosition(str, cursor))
//#define DEBUG std::cout << __FUNCTION__ << '\n' << CONTEXT << std::endl;
#define DEBUG

namespace joml {
namespace utf8 {
    constexpr bool is4CodeUnitLeader(char c)
    {
        return (c & 0b11111'000) == b4CodeUnitsLeader;
    }

    constexpr bool is3CodeUnitLeader(char c)
    {
        return (c & 0b1111'0000) == b3CodeUnitsLeader;
    }

    constexpr bool is2CodeUnitLeader(char c)
    {
        return (c & 0b111'00000) == b2CodeUnitsLeader;
    }

    constexpr bool isContinuationByte(char c)
    {
        return (c & 0b11'000000) == bContinuationByte;
    }

    constexpr size_t getCodePointLength(char firstCodeUnit)
    {
        if (is4CodeUnitLeader(firstCodeUnit))
            return 4;
        if (is3CodeUnitLeader(firstCodeUnit))
            return 3;
        if (is2CodeUnitLeader(firstCodeUnit))
            return 2;
        return 1;
    }

    std::string_view readCodePoint(std::string_view str, size_t& cursor)
    {
        const auto start = cursor;
        const auto num = getCodePointLength(str[start]);
        for (size_t i = 1; i < num; ++i) {
            // not a valid continuation byte => just end the sequence here, excluding continuation
            if (!isContinuationByte(str[start + i])) {
                cursor += i;
                return std::string_view(str.data() + start, i);
            }
        }
        cursor += num;
        return std::string_view(str.data() + start, num);
    }

    std::optional<std::string> encode(uint32_t codePoint)
    {
        auto c = [](int n) { return static_cast<char>(n); };
        if (codePoint <= 0x7f) {
            return std::string(1, static_cast<char>(codePoint));
        } else if (codePoint <= 0x7ff) {
            char buf[2] = {
                c(b2CodeUnitsLeader | ((0b11111'000000 & codePoint) >> 6)),
                c(bContinuationByte | ((0b00000'111111 & codePoint) >> 0)),
            };
            return std::string(buf, 2);
        } else if (codePoint <= 0xffff) {
            char buf[3] = {
                c(b3CodeUnitsLeader | ((0b1111'000000'000000 & codePoint) >> 12)),
                c(bContinuationByte | ((0b0000'111111'000000 & codePoint) >> 6)),
                c(bContinuationByte | ((0b0000'000000'111111 & codePoint) >> 0)),
            };
            return std::string(buf, 3);
        } else if (codePoint <= 0x1fffff) {
            char buf[4] = {
                c(b4CodeUnitsLeader | ((0b111'000000'000000'000000 & codePoint) >> 18)),
                c(bContinuationByte | ((0b000'111111'000000'000000 & codePoint) >> 12)),
                c(bContinuationByte | ((0b000'000000'111111'000000 & codePoint) >> 6)),
                c(bContinuationByte | ((0b000'000000'000000'111111 & codePoint) >> 0)),
            };
            return std::string(buf, 4);
        } else {
            return std::nullopt;
        }
    }

    std::optional<uint32_t> decode(std::string_view str)
    {
        auto contBits = [](char ch) { return ch & 0b00'111111; };
        switch (str.size()) {
        case 1:
            if (str[0] & 0b1'0000000) {
                return std::nullopt;
            }
            return static_cast<uint32_t>(str[0]);
            break;
        case 2:
            if (!is2CodeUnitLeader(str[0]) || !isContinuationByte(str[1])) {
                return std::nullopt;
            }
            return static_cast<uint32_t>(((str[0] & 0b000'11111) << 6) | contBits(str[1]));
            break;
        case 3:
            if (!is3CodeUnitLeader(str[0]) || !isContinuationByte(str[1])
                || !isContinuationByte(str[2])) {
                return std::nullopt;
            }
            return static_cast<uint32_t>(
                ((str[0] & 0b0000'1111) << 12) | (contBits(str[1]) << 6) | contBits(str[2]));
            break;
        case 4:
            if (!is4CodeUnitLeader(str[0]) || !isContinuationByte(str[1])
                || !isContinuationByte(str[2]) || !isContinuationByte(str[3])) {
                return std::nullopt;
            }
            return static_cast<uint32_t>(((str[0] & 0b00000'111) << 18) | (contBits(str[1]) << 12)
                | (contBits(str[2]) << 6) | contBits(str[3]));
            break;
        default:
            return std::nullopt;
        }
    }
}

std::string_view ParseResult::Error::asString(Type type)
{
    switch (type) {
    case ParseResult::Error::Type::Unspecified:
        return "Unspecified";
    case ParseResult::Error::Type::Unexpected:
        return "Unexpected";
    case ParseResult::Error::Type::InvalidKey:
        return "InvalidKey";
    case ParseResult::Error::Type::NoValue:
        return "NoValue";
    case ParseResult::Error::Type::CouldNotParseString:
        return "CouldNotParseString";
    case ParseResult::Error::Type::CouldNotParseHexNumber:
        return "CouldNotParseHexNumber";
    case ParseResult::Error::Type::CouldNotParseOctalNumber:
        return "CouldNotParseOctalNumber";
    case ParseResult::Error::Type::CouldNotParseBinaryNumber:
        return "CouldNotParseBinaryNumber";
    case ParseResult::Error::Type::CouldNotParseDecimalIntegerNumber:
        return "CouldNotParseDecimalIntegerNumber";
    case ParseResult::Error::Type::CouldNotParseFloatNumber:
        return "CouldNotParseFloatNumber";
    case ParseResult::Error::Type::InvalidValue:
        return "InvalidValue";
    case ParseResult::Error::Type::NoSeparator:
        return "NoSeparator";
    case ParseResult::Error::Type::ExpectedDictClose:
        return "ExpectedDictClose";
    case ParseResult::Error::Type::NotImplemented:
        return "NotImplemented";
    default:
        return "Unknown";
    }
}

std::string ParseResult::Error::string() const
{
    return std::string(asString(type)) + " at " + std::to_string(position.line) + ":"
        + std::to_string(position.column);
}

ParseResult::operator bool() const
{
    return std::holds_alternative<Node>(result);
}

const ParseResult::Error& ParseResult::error() const
{
    return std::get<Error>(result);
}

ParseResult::Error& ParseResult::error()
{
    return std::get<Error>(result);
}

const Node& ParseResult::node() const
{
    return std::get<Node>(result);
}

Node& ParseResult::node()
{
    return std::get<Node>(result);
}

namespace {
    bool isWhitespace(char ch)
    {
        return ch == '\t' || ch == ' ' || ch == '\n' || ch == '\r';
    }

    size_t skipWhitespace(std::string_view str, size_t& cursor)
    {
        const auto start = cursor;
        while (cursor < str.size() && isWhitespace(str[cursor]))
            cursor++;
        return cursor - start;
    }

    // quiet nan with arg = ""
    template <typename T>
    T nan()
    {
        constexpr auto isFloat = std::is_same_v<T, float>;
        constexpr auto isDouble = std::is_same_v<T, double>;
        static_assert(isFloat || isDouble);
        if constexpr (isFloat) {
            return std::nanf("");
        } else if constexpr (isDouble) {
            return std::nan("");
        }
    }

    Position getPosition(std::string_view str, size_t cursor)
    {
        size_t lineStart = 0;
        size_t line = 1;
        for (size_t i = 0; i < std::min(cursor, str.size()); ++i) {
            if (str[i] == '\n') {
                line++;
                lineStart = i;
            }
        }

        size_t colCursor = lineStart;
        size_t column = 1;
        while (colCursor < std::min(cursor, str.size())) {
            utf8::readCodePoint(str, colCursor);
            column++;
        }
        return Position { line, column };
    }

    ParseResult::Error makeError(ParseResult::Error::Type type, std::string_view str, size_t cursor)
    {
        return ParseResult::Error { type, getPosition(str, cursor) };
    }

    std::optional<uint32_t> parseHexEscape(std::string_view str)
    {
        if (str.find_first_not_of("0123456789abcdefABCDEF") != std::string_view::npos) {
            return std::nullopt;
        }
        try {
            size_t pos = 0;
            const auto num = std::stoul(std::string(str), &pos, 16);
            if (pos != str.size()) {
                return std::nullopt;
            }
            return num;
        } catch (const std::exception& exc) {
            return std::nullopt;
        }
    }

    std::optional<uint32_t> parseHexEscape(std::string_view str, size_t& cursor, size_t num)
    {
        if (cursor + num >= str.size()) {
            return std::nullopt;
        }
        const auto n = parseHexEscape(str.substr(cursor, num));
        if (!n) {
            return std::nullopt;
        }
        cursor += num;
        return *n;
    }

    std::optional<std::string> parseUnicodeHexEscape(
        std::string_view str, size_t& cursor, size_t num)
    {
        const auto codePoint = parseHexEscape(str, cursor, num);
        if (!codePoint) {
            return std::nullopt;
        }
        const auto s = utf8::encode(*codePoint);
        if (!s) {
            return std::nullopt;
        }
        return *s;
    }

    std::optional<std::string> parseString(std::string_view str, size_t& cursor)
    {
        DEBUG;
        assert(str[cursor] == '"');
        std::string ret;
        ret.reserve(32);
        cursor++;
        while (cursor < str.size()) {
            if (str[cursor] == '\\') {
                if (cursor >= str.size()) {
                    return std::nullopt;
                }
                cursor++;
                const auto c = str[cursor];
                switch (c) {
                case '\\':
                    ret.append(1, '\\');
                    cursor++;
                    break;
                case '"':
                    ret.append(1, '"');
                    cursor++;
                    break;
                case 'b':
                    ret.append(1, '\b');
                    cursor++;
                    break;
                case 'f':
                    ret.append(1, '\f');
                    cursor++;
                    break;
                case 'n':
                    ret.append(1, '\n');
                    cursor++;
                    break;
                case 'r':
                    ret.append(1, '\r');
                    cursor++;
                    break;
                case 't':
                    ret.append(1, '\t');
                    cursor++;
                    break;
                case 'x': { // Should this be another unicode escape?
                    cursor++;
                    const auto c = parseHexEscape(str, cursor, 2);
                    if (!c) {
                        return std::nullopt;
                    }
                    ret.append(1, static_cast<char>(*c));
                    break;
                }
                case 'u': {
                    cursor++;
                    const auto s = parseUnicodeHexEscape(str, cursor, 4);
                    if (!s) {
                        return std::nullopt;
                    }
                    ret.append(*s);
                    break;
                }
                case 'U': {
                    cursor++;
                    const auto s = parseUnicodeHexEscape(str, cursor, 8);
                    if (!s) {
                        return std::nullopt;
                    }
                    ret.append(*s);
                    break;
                }
                default:
                    return std::nullopt;
                }
            } else if (str[cursor] == '"') {
                cursor++; // Advance past closing quote
                return ret;
            } else {
                ret.append(1, str[cursor]);
                cursor++;
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> parseKey(std::string_view str, size_t& cursor)
    {
        DEBUG;
        skipWhitespace(str, cursor);
        if (cursor == str.size()) {
            return std::nullopt;
        }
        size_t newCursor = cursor;
        if (str[newCursor] == '"') {
            const auto s = parseString(str, newCursor);
            if (!s) {
                return std::nullopt;
            }
            skipWhitespace(str, newCursor);
            if (str[newCursor] != ':') {
                return std::nullopt;
            }
            newCursor++;
            return *s;
        } else {
            while (str[newCursor] != ':') {
                newCursor++;
                if (newCursor >= str.size()) {
                    return std::nullopt;
                }
            }
            const auto key = str.substr(cursor, newCursor - cursor);
            if (key.empty()) {
                return std::nullopt;
            }
            newCursor++; // skip ':'
            cursor = newCursor;
            return std::string(key);
        }
    }

    std::optional<Node::Integer> parseInteger(std::string_view str, int base)
    {
        try {
            size_t pos = 0;
            const auto num = std::stoll(std::string(str), &pos, base);
            if (pos != str.size()) {
                return std::nullopt;
            }
            return num;
        } catch (const std::exception& exc) {
            return std::nullopt;
        }
    }

    std::optional<Node::Float> parseFloat(std::string_view str)
    {
        try {
            size_t pos = 0;
            const auto num = std::stof(std::string(str), &pos);
            if (pos != str.size()) {
                return std::nullopt;
            }
            return num;
        } catch (const std::exception& exc) {
            return std::nullopt;
        }
    }

    ParseResult parseNumber(std::string_view str, size_t cursor, size_t cursorEnd)
    {
        DEBUG;
        // must be a number of some kind
        const int sign = str[cursor] == '-' ? -1 : 1;
        if (str[cursor] == '+' || str[cursor] == '-') {
            cursor++;
        }
        const auto value = str.substr(cursor, cursorEnd - cursor);

        if (value == "inf") {
            return Node(sign * std::numeric_limits<Node::Float>::infinity());
        } else if (value == "nan") {
            return nan<Node::Float>();
        }

        const auto prefix = value.substr(0, 2);
        if (prefix == "0x") {
            const auto n = parseInteger(value.substr(2), 16);
            if (!n) {
                return makeError(ParseResult::Error::Type::CouldNotParseHexNumber, str, cursor);
            }
            return Node(sign * *n);
        } else if (prefix == "0o") {
            const auto n = parseInteger(value.substr(2), 8);
            if (!n) {
                return makeError(ParseResult::Error::Type::CouldNotParseOctalNumber, str, cursor);
            }
            return Node(sign * *n);
        } else if (prefix == "0b") {
            const auto n = parseInteger(value.substr(2), 2);
            if (!n) {
                return makeError(ParseResult::Error::Type::CouldNotParseBinaryNumber, str, cursor);
            }
            return Node(sign * *n);
        }

        // all digits
        if (value.find_first_not_of("0123456789") == std::string::npos) {
            const auto n = parseInteger(value, 10);
            if (!n) {
                return makeError(
                    ParseResult::Error::Type::CouldNotParseDecimalIntegerNumber, str, cursor);
            }
            return Node(sign * *n);
        }

        if (value.find_first_not_of("0123456789.eE+-") == std::string::npos) {
            const auto n = parseFloat(value);
            if (!n) {
                return makeError(ParseResult::Error::Type::CouldNotParseFloatNumber, str, cursor);
            }
            return Node(sign * *n);
        }

        return makeError(ParseResult::Error::Type::InvalidValue, str, cursor);
    }

    ParseResult parseArray(std::string_view str, size_t& cursor);
    ParseResult parseDictionary(std::string_view str, size_t& cursor, bool isRoot = false);

    ParseResult parseNode(std::string_view str, size_t& cursor)
    {
        DEBUG;
        skipWhitespace(str, cursor);
        if (cursor == str.size())
            return makeError(ParseResult::Error::Type::NoValue, str, cursor);
        if (str[cursor] == '{') {
            cursor++;
            return parseDictionary(str, cursor);
        } else if (str[cursor] == '[') {
            cursor++;
            return parseArray(str, cursor);
        } else if (str[cursor] == '"') {
            const auto s = parseString(str, cursor);
            if (!s) {
                return makeError(ParseResult::Error::Type::CouldNotParseString, str, cursor);
            }
            return Node(*s);
        } else {
            const auto valueChars
                = "0123456789abcdefghijlmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.+-";
            auto valueEnd = str.find_first_not_of(valueChars, cursor);
            if (valueEnd == std::string_view::npos) {
                valueEnd = str.size();
            }
            const auto value = str.substr(cursor, valueEnd - cursor);
            if (value.empty()) {
                return makeError(ParseResult::Error::Type::NoValue, str, cursor);
            }

            if (value == "null") {
                cursor += value.size();
                return Node(Node::Null {});
            } else if (value == "true") {
                cursor += value.size();
                return Node(true);
            } else if (value == "false") {
                cursor += value.size();
                return Node(false);
            }

            auto node = parseNumber(str, cursor, valueEnd);
            if (!node) {
                return node;
            }
            cursor += value.size();
            return node;
        }
    }

    bool skipSeparator(std::string_view str, size_t& cursor)
    {
        bool separatorFound = false;
        while (cursor < str.size()) {
            if (str[cursor] == ',' || str[cursor] == '\n') {
                separatorFound = true;
            } else if (!isWhitespace(str[cursor])) {
                break;
            }
            cursor++;
        }
        return separatorFound;
    }

    ParseResult parseArray(std::string_view str, size_t& cursor)
    {
        DEBUG;
        Node::Array arr;
        while (cursor < str.size()) {
            auto value = parseNode(str, cursor);
            if (!value) {
                return value;
            }
            arr.emplace_back(std::make_unique<Node>(std::move(value.node())));

            const auto separatorFound = skipSeparator(str, cursor);

            if (str[cursor] == ']') {
                cursor++; // skip ']'
                break;
            }

            if (!separatorFound) {
                return makeError(ParseResult::Error::Type::NoSeparator, str, cursor);
            }
        }
        return Node(std::move(arr));
    }

    ParseResult parseDictionary(std::string_view str, size_t& cursor, bool isRoot)
    {
        DEBUG;
        Node::Dictionary dict;
        while (cursor < str.size()) {
            const auto key = parseKey(str, cursor);
            if (!key) {
                return makeError(ParseResult::Error::Type::InvalidKey, str, cursor);
            }

            auto value = parseNode(str, cursor);
            if (!value) {
                return value;
            }
            dict.emplace_back(std::move(*key), std::make_unique<Node>(std::move(value.node())));

            const auto separatorFound = skipSeparator(str, cursor);

            if (str[cursor] == '}') {
                cursor++; // skip '}'
                break;
            }

            if (!separatorFound) {
                return makeError(ParseResult::Error::Type::NoSeparator, str, cursor);
            }

            assert(cursor <= str.size());
            if (cursor == str.size() && !isRoot) {
                return makeError(ParseResult::Error::Type::ExpectedDictClose, str, cursor);
            }
        }
        return Node(std::move(dict));
    }
}

// inefficient
std::string getContextString(std::string_view str, const Position& position)
{
    std::vector<size_t> lineStarts { 0 };
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\n') {
            lineStarts.push_back(i + 1);
        }
    }

    const int numLines = lineStarts.size();
    const int numContextLines = 1;
    const auto startLine = std::max(1, static_cast<int>(position.line) - numContextLines);
    const auto endLine = std::min(numLines, static_cast<int>(position.line) + numContextLines);
    std::string ret;
    for (int i = startLine; i <= endLine; ++i) {
        const auto idx = i - 1;
        const auto start = lineStarts[idx];
        const auto end = i < numLines ? lineStarts[idx + 1] - 1 : str.size();
        ret.append(str.substr(start, end - start));
        ret.append("\n");
        if (i == static_cast<int>(position.line)) {
            ret.append(position.column - 1, ' ');
            ret.append("^\n");
        }
    }
    return ret;
}

ParseResult parse(std::string_view str)
{
    size_t cursor = 0;
    return parseDictionary(str, cursor, true);
}

} // namespace joml
