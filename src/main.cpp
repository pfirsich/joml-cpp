#include <cassert>
#include <iostream>

#include "joml.hpp"

using namespace std::literals;

const auto doc = R"(
key1: "value1"
key2: "value2"
key2: true
key2: false
key2: 12
key2: 1.24
dict: {
    a: 1
    b: 2
}
)"sv;

const auto singleIndent = "    ";

std::string getIndent(size_t depth)
{
    std::string ret;
    for (size_t i = 0; i < depth; ++i)
        ret.append(singleIndent);
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
    } else if (node.is<joml::Node::Bool>()) {
        return node.as<joml::Node::Bool>() ? "true" : "false";
    } else if (node.is<joml::Node::Integer>()) {
        return std::to_string(node.as<joml::Node::Integer>());
    } else if (node.is<joml::Node::Float>()) {
        return std::to_string(node.as<joml::Node::Float>());
    } else if (node.is<joml::Node::String>()) {
        return "\"" + node.as<joml::Node::String>() + "\"";
    } else {
        return "UNIMPLEMENTED";
    }
}

int main()
{
    auto res = joml::parse(doc);
    if (!res) {
        const auto err = res.error();
        std::cerr << "Error: " << err.string() << std::endl;
        std::cerr << joml::getContextString(doc, err.position) << std::endl;
    }
    std::cout << toJson(res.node()) << std::endl;
}
