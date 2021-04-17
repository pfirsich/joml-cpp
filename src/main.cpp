#include <cassert>
#include <iostream>

#include "joml.hpp"

using namespace std::literals;

/*
 * TODO:
 * - Comments
 * - Handle unexpected end of files better
 * - Escaping backslash in strings
 * - Arbitrary byte sequences in strings
 * - Trimming lines in multi line strings
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

struct ErrorTest {
    std::string_view title;
    std::string_view jomlSource;
    joml::ParseResult::Error::Type errorType;
};

struct SuccessTest {
    std::string_view title;
    std::string_view jomlSource;
    std::string_view jsonSource;
};

const std::vector<ErrorTest> errorTests {
    {
        "Missing separator at end",
        "dict: {a: 1, b: 2, c: 3",
        joml::ParseResult::Error::Type::NoSeparator,
    },
    {
        "Missing separator in between",
        "dict: {a: 1 b: 2, c: 3",
        joml::ParseResult::Error::Type::NoSeparator,
    },
    {
        "Non-terminated dict",
        "dict: {a: 1, b: 2, c: 3,",
        joml::ParseResult::Error::Type::ExpectedDictClose,
    },
};

const std::vector<SuccessTest> successTests {
    {
        "Big test",
        R"(
key1: "value1"
key2: "value2"
key2: true
key2: false
key2: 12
key2: 1.24
key2: 0xf
dict: {
    a: 1
    b: {
        c: 10,
        d: 11
    }
}
position: {x: 0, y: 1}
values: [false, 1, "two", 3, 4.0]
)",
        R"({
    "key1": "value1",
    "key2": "value2",
    "key2": true,
    "key2": false,
    "key2": 12,
    "key2": 1.240000,
    "key2": 15,
    "dict": {
        "a": 1,
        "b": {
            "c": 10,
            "d": 11
        }
    },
    "position": {
        "x": 0,
        "y": 1
    },
    "values": [
        false,
        1,
        "two",
        3,
        4.000000
    ]
})",
    },
};

class TestSuite {
public:
    TestSuite(
        const std::vector<ErrorTest>& errorTests, const std::vector<SuccessTest>& successTests)
        : errorTests_(errorTests)
        , successTests_(successTests)
    {
    }

    void run(const ErrorTest& test)
    {
        title(test.title);
        const auto res = joml::parse(test.jomlSource);
        if (res) {
            fail(test.title, "parse successful");
        } else {
            const auto err = res.error();
            if (err.type != test.errorType) {
                fail(test.title, "wrong error: " + err.string());
                std::cerr << joml::getContextString(test.jomlSource, err.position) << std::endl;
            } else {
                pass();
            }
        }
    }

    void run(const SuccessTest& test)
    {
        title(test.title);
        const auto res = joml::parse(test.jomlSource);
        if (!res) {
            const auto err = res.error();
            fail(test.title, "parse error: " + err.string());
            std::cerr << joml::getContextString(test.jomlSource, err.position) << std::endl;
        }
        const auto json = toJson(res.node());
        if (json != test.jsonSource) {
            fail(test.title, "wrong parse result");
            std::cerr << json << std::endl;
        } else {
            pass();
        }
    }

    void run()
    {
        for (const auto& test : errorTests_)
            run(test);
        for (const auto& test : successTests_)
            run(test);
    }

    void run(std::string_view title)
    {
        for (const auto& test : errorTests_) {
            if (test.title == title) {
                run(test);
                return;
            }
        }
        for (const auto& test : successTests_) {
            if (test.title == title) {
                run(test);
                return;
            }
        }
        std::cerr << "Test '" << title << "' not found" << std::endl;
        assert(false);
    }

    const std::vector<std::string>& getFailedTests() const
    {
        return failedTests_;
    }

    void printSummary() const
    {
        if (!failedTests_.empty()) {
            std::cout << failedTests_.size() << " tests failed:" << std::endl;
            for (const auto& title : failedTests_) {
                std::cout << title << std::endl;
            }
        }
    }

private:
    void title(std::string_view title) const
    {
        std::cout << "# " << title << ": ";
    }

    void pass() const
    {
        std::cout << "\x1b[32mPASS\x1b[0m" << std::endl;
    }

    void fail(std::string_view title, std::string_view message)
    {
        std::cout << "\x1b[31mFAIL\x1b[0m"
                  << " (" << message << ")" << std::endl;
        failedTests_.emplace_back(title);
    }

    std::vector<ErrorTest> errorTests_;
    std::vector<SuccessTest> successTests_;
    std::vector<std::string> failedTests_;
};

int main()
{
    TestSuite suite(errorTests, successTests);
    suite.run();
    suite.printSummary();
    return suite.getFailedTests().size();
}
