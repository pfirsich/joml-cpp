#include <cassert>
#include <cmath>
#include <iostream>

#include "joml.hpp"

using namespace std::literals;

/*
 * TODO:
 * - Comments
 * - Handle unexpected end of files better
 * - Properly implement escape characters
 * - \x escapes
 * - \u and \U escapes
 * - inf/nan
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
        // TODO: Make this emit \x for non-ascii (visible) characters
        return "\"" + node.as<joml::Node::String>() + "\"";
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

    const auto res = joml::parse(*source);
    if (!res) {
        const auto err = res.error();
        std::cerr << "Error parsing JOML file: " << err.string() << std::endl;
        std::cerr << joml::getContextString(*source, err.position) << std::endl;
        return 2;
    }

    std::cout << toJson(res.node()) << std::endl;
    return 0;
}
