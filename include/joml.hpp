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
    struct Null {
    };
    using String = std::string;
    using Bool = bool;
    using Integer = int64_t;
    using Float = double;
    using Array = std::vector<std::unique_ptr<Node>>;
    using Dictionary = std::vector<std::pair<std::string, std::unique_ptr<Node>>>;

    template <typename T>
    Node(T&& arg)
        : data_(std::forward<T>(arg))
    {
    }

    Node(Node&&) = default;

    template <typename T>
    bool is() const
    {
        return std::holds_alternative<T>(data_);
    }

    template <typename T>
    const T& as() const
    {
        return std::get<T>(data_);
    }

private:
    std::variant<Null, String, Bool, Integer, Float, Array, Dictionary> data_;
};

struct Position {
    size_t line;
    size_t column;
};

class ParseResult {
public:
    struct Error {
        enum class Type {
            Unspecified,
            Unexpected,
            InvalidKey,
            NoValue,
            CouldNotParseString,
            CouldNotParseHexNumber,
            CouldNotParseOctalNumber,
            CouldNotParseBinaryNumber,
            CouldNotParseDecimalIntegerNumber,
            CouldNotParseFloatNumber,
            InvalidValue,
            NoSeparator,
            ExpectedDictClose,
            InvalidEscape,
            NotImplemented,
        };

        Type type;
        Position position;

        static std::string_view asString(Type type);

        std::string string() const;
    };

    std::variant<Node, Error> result;

    template <typename T>
    ParseResult(T&& arg)
        : result(std::forward<T>(arg))
    {
    }

    ParseResult(ParseResult&&) = default;

    explicit operator bool() const;

    const Error& error() const;
    Error& error();

    const Node& node() const;
    Node& node();
};

std::string getContextString(std::string_view str, const Position& position);

ParseResult parse(std::string_view str);

} // namespace joml
