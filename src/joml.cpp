#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "joml.hpp"

namespace joml {
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
    case ParseResult::Error::Type::NoSeparator:
        return "NoSeparator";
    case ParseResult::Error::Type::NotImplemented:
        return "NotImplemented";
    default:
        return "Unknown";
    }
}

std::string ParseResult::Error::string() const
{
    std::stringstream ss;
    ss << asString(type);
    ss << " at " << position.line << ":" << position.column;
    return ss.str();
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

    size_t getCodePointLength(char ch)
    {
        if ((ch & 0b11111000) == 0b11110000)
            return 4;
        if ((ch & 0b11110000) == 0b11100000)
            return 3;
        if ((ch & 0b11100000) == 0b11000000)
            return 2;
        return 1;
    }

    std::string_view readCodePoint(std::string_view str, size_t& cursor)
    {
        const auto num = getCodePointLength(str[cursor]);
        if (num == 1) {
            cursor++;
            return std::string_view(str.data() + cursor, 1);
        }
        for (size_t i = 1; i < num; ++i) {
            // not a valid continuation byte => just end the sequence here
            if ((str[cursor + i] & 0b11000000) != 0b10000000)
                return std::string_view(str.data() + cursor, i);
            cursor++;
        }
        return std::string_view(str.data() + cursor, num);
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
            readCodePoint(str, colCursor);
            column++;
        }
        return Position { line, column };
    }

    ParseResult::Error makeError(ParseResult::Error::Type type, std::string_view str, size_t cursor)
    {
        return ParseResult::Error { type, getPosition(str, cursor) };
    }

    std::optional<std::string> parseString(std::string_view str, size_t& cursor)
    {
        assert(str[cursor] == '"');
        const auto start = cursor + 1;
        while (cursor < str.size()) {
            cursor++;
            if (str[cursor] == '"' && str[cursor] != '\\') {
                const auto s = str.substr(start, cursor - start);
                cursor++; // Advance past closing quote
                return std::string(s);
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> parseKey(std::string_view str, size_t& cursor)
    {
        skipWhitespace(str, cursor);
        if (cursor == str.size()) {
            std::cout << "parseKey: cursor at end" << std::endl;
            return std::nullopt;
        }
        size_t newCursor = cursor;
        if (str[newCursor] == '"') {
            const auto s = parseString(str, newCursor);
            if (!s) {
                std::cout << "Could not parse string" << std::endl;
                return std::nullopt;
            }
            skipWhitespace(str, newCursor);
            if (str[newCursor] != ':') {
                std::cout << "No colon after key" << std::endl;
                return std::nullopt;
            }
            newCursor++;
            return *s;
        } else {
            while (str[newCursor] != ':') {
                newCursor++;
                if (newCursor >= str.size()) {
                    std::cout << "Could not find ':'" << std::endl;
                    return std::nullopt;
                }
            }
            const auto key = str.substr(cursor, newCursor - cursor);
            newCursor++; // skip ':'
            cursor = newCursor;
            return std::string(key);
        }
    }

    std::optional<Node::Integer> parseInteger(std::string_view str, int base)
    {
        const auto s = std::string(str);
        size_t pos = 0;
        try {
            const auto num = std::stoll(s, &pos, base);
            if (pos != s.size()) {
                return std::nullopt;
            }
            return num;
        } catch (const std::invalid_argument& exc) {
            return std::nullopt;
        } catch (const std::out_of_range& exc) {
            return std::nullopt;
        }
    }

    std::optional<Node::Float> parseFloat(std::string_view str)
    {
        const auto s = std::string(str);
        size_t pos = 0;
        try {
            const auto num = std::stof(s, &pos);
            if (pos != s.size()) {
                return std::nullopt;
            }
            return num;
        } catch (const std::invalid_argument& exc) {
            return std::nullopt;
        } catch (const std::out_of_range& exc) {
            return std::nullopt;
        }
    }

    ParseResult parseDictionary(std::string_view str, size_t& cursor);

    ParseResult parseNode(std::string_view str, size_t& cursor)
    {
        skipWhitespace(str, cursor);
        if (cursor == str.size())
            return makeError(ParseResult::Error::Type::NoValue, str, cursor);
        if (str[cursor] == '{') {
            cursor++;
            std::cout << "Dict" << std::endl;
            return parseDictionary(str, cursor);
        } else if (str[cursor] == '[') {
            std::cout << "Array" << std::endl;
            return makeError(ParseResult::Error::Type::NotImplemented, str, cursor);
        } else if (str[cursor] == '"') {
            const auto s = parseString(str, cursor);
            if (!s) {
                return makeError(ParseResult::Error::Type::CouldNotParseString, str, cursor);
            }
            std::cout << "String: '" << *s << "'" << std::endl;
            return Node(*s);
        } else {
            auto valueEnd = cursor;
            while (valueEnd < str.size() && !isWhitespace(str[valueEnd]) && str[valueEnd] != ',') {
                valueEnd++;
            }
            const auto value = str.substr(cursor, valueEnd - cursor);
            std::cout << "Value: '" << value << "'" << std::endl;

            if (value == "true") {
                std::cout << "true" << std::endl;
                cursor += value.size();
                return Node(true);
            } else if (value == "false") {
                std::cout << "false" << std::endl;
                cursor += value.size();
                return Node(false);
            }

            // must be a number of some kind
            const int sign = value[0] == '-' ? -1 : 1;
            if (value[0] == '+' || value[0] == '-') {
                cursor++;
            }

            const auto prefix = value.substr(0, 2);
            if (prefix == "0x") {
                const auto n = parseInteger(value.substr(2), 16);
                if (!n) {
                    return makeError(ParseResult::Error::Type::CouldNotParseHexNumber, str, cursor);
                }
                cursor += value.size();
                return Node(sign * *n);
            } else if (prefix == "0o") {
                const auto n = parseInteger(value.substr(2), 8);
                if (!n) {
                    return makeError(
                        ParseResult::Error::Type::CouldNotParseOctalNumber, str, cursor);
                }
                cursor += value.size();
                return Node(sign * *n);
            } else if (prefix == "0b") {
                const auto n = parseInteger(value.substr(2), 2);
                if (!n) {
                    return makeError(
                        ParseResult::Error::Type::CouldNotParseBinaryNumber, str, cursor);
                }
                cursor += value.size();
                return Node(sign * *n);
            }

            // all digits
            if (value.find_first_not_of("0123456789") == std::string::npos) {
                const auto n = parseInteger(value, 10);
                cursor += value.size();
                return Node(sign * *n);
            }

            const auto n = parseFloat(value);
            if (!n) {
                return makeError(ParseResult::Error::Type::CouldNotParseFloatNumber, str, cursor);
            }
            cursor += value.size();
            return Node(sign * *n);
        }
    }

    ParseResult parseDictionary(std::string_view str, size_t& cursor)
    {
        Node::Dictionary dict;
        while (cursor < str.size()) {
            const auto key = parseKey(str, cursor);
            if (!key) {
                return makeError(ParseResult::Error::Type::InvalidKey, str, cursor);
            }
            std::cout << "Key: '" << *key << "'" << std::endl;

            auto value = parseNode(str, cursor);
            if (!value) {
                return value;
            }

            dict.emplace_back(std::move(*key), std::make_unique<Node>(std::move(value.node())));

            bool separatorFound = false;
            bool dictFinished = false;
            while (cursor < str.size()) {
                if (str[cursor] == '}') {
                    dictFinished = true;
                    break;
                } else if (str[cursor] == ',' || str[cursor] == '\n') {
                    separatorFound = true;
                } else if (!isWhitespace(str[cursor])) {
                    break;
                }
                cursor++;
            }

            if (dictFinished) {
                cursor++; // skip '}'
                break;
            }

            if (!separatorFound) {
                return makeError(ParseResult::Error::Type::NoSeparator, str, cursor);
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
        const auto end = i < numLines ? lineStarts[idx + 1] : str.size();
        ret.append(str.substr(start, end - start));
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
    return parseDictionary(str, cursor);
}

} // namespace joml
