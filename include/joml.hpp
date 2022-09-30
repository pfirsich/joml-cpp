#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace joml {

namespace utf8 {
    constexpr auto b4CodeUnitsLeader = 0b11110'000;
    constexpr auto b3CodeUnitsLeader = 0b1110'0000;
    constexpr auto b2CodeUnitsLeader = 0b110'00000;
    constexpr auto bContinuationByte = 0b10'000000;

    constexpr bool is4CodeUnitLeader(char c);
    constexpr bool is2CodeUnitLeader(char c);
    constexpr bool is3CodeUnitLeader(char c);
    constexpr bool isContinuationByte(char c);
    constexpr size_t getCodePointLength(char firstCodeUnit);
    std::optional<std::string> encode(uint32_t codePoint);
    std::optional<uint32_t> decode(std::string_view str);
    std::string_view readCodePoint(std::string_view str, size_t& cursor);
}

class Node {
public:
    struct Invalid { };
    struct Null { };
    using String = std::string;
    using Bool = bool;
    using Integer = int64_t;
    using Float = double;
    using Array = std::vector<Node>;
    using Dictionary = std::vector<std::pair<std::string, Node>>;

    Node() : data_(Invalid {}) { }
    Node(Null v) : data_(std::move(v)) { }
    Node(String v) : data_(std::move(v)) { }
    Node(Bool v) : data_(v) { }
    Node(Integer v) : data_(v) { }
    Node(Float v) : data_(v) { }
    Node(Array v) : data_(std::move(v)) { }
    Node(Dictionary v) : data_(std::move(v)) { }

    Node(Node&&) = default;
    Node(const Node&) = default;

    template <typename T>
    bool is() const
    {
        if constexpr (std::is_same_v<T, Float>) {
            return std::holds_alternative<Float>(data_) || std::holds_alternative<Integer>(data_);
        }
        return std::holds_alternative<T>(data_);
    }

    bool isValid() const { return !is<Invalid>(); }
    bool isNull() const { return is<Null>(); }
    bool isString() const { return is<String>(); }
    bool isBool() const { return is<Bool>(); }
    bool isInteger() const { return is<Integer>(); }
    bool isFloat() const { return is<Float>() || is<Integer>(); }
    bool isArray() const { return is<Array>(); }
    bool isDictionary() const { return is<Dictionary>(); }

    operator bool() const { return isValid(); }

    template <typename T>
    const T& as() const
    {
        return std::get<T>(data_);
    }

    const String& asString() const { return as<String>(); }
    const Bool& asBool() const { return as<Bool>(); }
    const Integer& asInteger() const { return as<Integer>(); }
    const Float& asFloat() const { return as<Float>(); }
    const Array& asArray() const { return as<Array>(); }
    const Dictionary& asDictionary() const { return as<Dictionary>(); }

    size_t size() const
    {
        if (isInteger() || isFloat() || isString() || isBool()) {
            return 1;
        } else if (isArray()) {
            return asArray().size();
        } else if (isDictionary()) {
            return asDictionary().size();
        } else {
            return 0;
        }
    }

    const Node& operator[](std::string_view key) const
    {
        if (isDictionary()) {
            for (const auto& [k, v] : asDictionary()) {
                if (k == key) {
                    return v;
                }
            }
        }
        return getInvalidNode();
    }

    const Node& operator[](size_t idx) const
    {
        if (isArray() && idx < asArray().size()) {
            return asArray()[idx];
        }
        return getInvalidNode();
    }

private:
    static const Node& getInvalidNode()
    {
        static Node node;
        return node;
    }

    std::variant<Invalid, Null, String, Bool, Integer, Float, Array, Dictionary> data_;
};

struct Position {
    size_t line;
    size_t column;
};

struct ParseError {
    enum class Type {
        Unspecified,
        Unexpected,
        InvalidKey,
        NoValue,
        CouldNotParseHexNumber,
        CouldNotParseOctalNumber,
        CouldNotParseBinaryNumber,
        CouldNotParseDecimalIntegerNumber,
        CouldNotParseFloatNumber,
        InvalidValue,
        NoSeparator,
        ExpectedDictClose,
        ExpectedKey,
        ExpectedColon,
        UnterminatedString,
        InvalidEscape,
    };

    Type type;
    Position position;

    std::string string() const;
};

std::string_view asString(ParseError::Type type);

template <typename T>
struct ParseResult {
    std::variant<T, ParseError> result;

    template <typename U>
    ParseResult(U&& arg) : result(std::forward<U>(arg))
    {
    }

    ParseResult(ParseResult&&) = default;

    explicit operator bool() const { return std::holds_alternative<T>(result); }

    const ParseError& error() const { return std::get<ParseError>(result); }

    ParseError& error() { return std::get<ParseError>(result); }

    const T& operator*() const { return std::get<T>(result); }

    T& operator*() { return std::get<T>(result); }
};

std::string getContextString(std::string_view str, const Position& position);

ParseResult<Node::Dictionary> parse(std::string_view str);

} // namespace joml
