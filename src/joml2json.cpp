#include <cassert>
#include <cmath>
#include <iostream>

#include "joml.hpp"

using namespace std::literals;

/*
 * TODO:
 * - Replace some optionals with new result type to provide more errors
 * - Handle unexpected end of files better (check whether cursor is < size for every str[cursor])
 * - Trimming lines in multi line strings
 * - Test cases for every branch and every part of the spec
 */

std::string getIndent(size_t depth)
{
    std::string ret;
    for (size_t i = 0; i < depth; ++i)
        ret.append("    ");
    return ret;
}

std::optional<std::string> escape(std::string_view str)
{
    std::string escaped;
    escaped.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        const auto c = str[i];
        switch (c) {
        case '\\':
            escaped.append("\\\\");
            break;
        case '"':
            escaped.append("\\\"");
            break;
        case '\b':
            escaped.append("\\b");
            break;
        case '\f':
            escaped.append("\\f");
            break;
        case '\n':
            escaped.append("\\n");
            break;
        case '\r':
            escaped.append("\\r");
            break;
        case '\t':
            escaped.append("\\t");
            break;
        default:
            if (c < 0x00 || (c >= 0x20 && c != 0x7f)) {
                escaped.append(1, c);
            } else {
                const auto codePointStr = joml::utf8::readCodePoint(str, i);
                const auto codePoint = joml::utf8::decode(codePointStr);
                if (!codePoint) {
                    return std::nullopt;
                }

                escaped.append("\\u");
                const size_t numDigits = 4;

                static constexpr std::array<char, 16> hexDigits { '0', '1', '2', '3', '4', '5', '6',
                    '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
                for (size_t i = 0; i < numDigits; ++i) {
                    const auto shift = (numDigits - i - 1) * 4;
                    escaped.append(1, hexDigits[(*codePoint >> shift) & 0xf]);
                }

                // readCodePoint has already advanced past the code point,
                // so we need to compensate for the ++i of the for loop
                i--;
            }
        }
    }
    return escaped;
}

std::string toJson(const joml::Node& node, size_t depth = 0)
{
    if (node.is<joml::Node::Dictionary>()) {
        std::string ret;
        const auto indent = getIndent(depth + 1);
        ret.append("{\n");
        const auto& dict = node.as<joml::Node::Dictionary>();
        for (size_t i = 0; i < dict.size(); ++i) {
            ret.append(indent + "\"" + dict[i].first + "\": ");
            ret.append(toJson(*dict[i].second, depth + 1) + (i < dict.size() - 1 ? ",\n" : "\n"));
        }
        ret.append(getIndent(depth) + "}");
        return ret;
    } else if (node.is<joml::Node::Array>()) {
        std::string ret;
        const auto indent = getIndent(depth + 1);
        ret.append("[\n");
        const auto& arr = node.as<joml::Node::Array>();
        for (size_t i = 0; i < arr.size(); ++i) {
            ret.append(indent + toJson(*arr[i], depth + 1) + (i < arr.size() - 1 ? ",\n" : "\n"));
        }
        ret.append(getIndent(depth) + "]");
        return ret;
    } else if (node.is<joml::Node::Null>()) {
        return "null";
    } else if (node.is<joml::Node::Bool>()) {
        return node.as<joml::Node::Bool>() ? "true" : "false";
    } else if (node.is<joml::Node::Integer>()) {
        return std::to_string(node.as<joml::Node::Integer>());
    } else if (node.is<joml::Node::Float>()) {
        const auto f = node.as<joml::Node::Float>();
        if (std::isnan(f)) {
            return "NaN";
        } else if (std::isinf(f)) {
            return std::signbit(f) ? "-Infinity" : "Infinity";
        }
        return std::to_string(f);
    } else if (node.is<joml::Node::String>()) {
        const auto& str = node.as<joml::Node::String>();
        const auto escaped = escape(str);
        if (!escaped) {
            std::cerr << "Could not escape string: '" << str << "'" << std::endl;
            std::exit(1);
        }
        return "\"" + *escaped + "\"";
    } else {
        assert(false && "Invalid node type");
    }
}

std::optional<std::string> readFile(const std::string& path)
{
    FILE* f = ::fopen(path.c_str(), "r");
    if (!f) {
        std::cerr << "Could not open file" << std::endl;
        return std::nullopt;
    }
    ::fseek(f, 0, SEEK_END);
    const auto size = ::ftell(f);
    if (size < 0) {
        std::cerr << "Could not get file size" << std::endl;
        return std::nullopt;
    }
    ::fseek(f, 0, SEEK_SET);
    std::string str(size, '\0');
    const auto n = ::fread(str.data(), 1, size, f);
    if (n < static_cast<size_t>(size)) {
        std::cerr << "Could not read file" << std::endl;
        std::cerr << "Read " << n << " of " << size << " characters" << std::endl;
        return std::nullopt;
    }
    ::fclose(f);
    return str;
}

int main(int argc, char** argv)
{
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        std::cerr << "Mandatory argument (JOML file) missing" << std::endl;
        return 1;
    }
    const auto path = args[0];
    const auto source = readFile(path);
    if (!source) {
        return 1;
    }

    auto res = joml::parse(*source);
    if (!res) {
        const auto err = res.error();
        std::cerr << "Error parsing JOML file: " << err.string() << std::endl;
        std::cerr << joml::getContextString(*source, err.position) << std::endl;
        return 2;
    }

    std::cout << toJson(joml::Node(std::move(res.value()))) << std::endl;
    return 0;
}
