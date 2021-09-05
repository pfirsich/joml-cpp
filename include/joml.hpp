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

namespace detail {
    template <typename T, typename V>
    struct isVariantType : std::false_type {
    };

    template <typename T, typename... Vs>
    struct isVariantType<T, std::variant<Vs...>>
        : std::bool_constant<(std::is_same_v<T, Vs> || ...)> {
    };
}

class Node {
public:
    struct Null {
    };
    using String = std::string;
    using Bool = bool;
    using Integer = int64_t;
    using Float = double;
    using Array = std::vector<Node>;
    using Dictionary = std::vector<std::pair<std::string, Node>>;

    using Variant = std::variant<Null, String, Bool, Integer, Float, Array, Dictionary>;

    Node(Node&&) = default;

    Node(const Node&) = default;

    // This silly massaging is required to appease the devilish MSVC
    // clang will simply compile this without any enable_if.
    template <typename T,
        typename = std::enable_if_t<detail::isVariantType<std::decay_t<T>, Variant>::value>>
    explicit Node(T&& arg)
        : data_(std::forward<T>(arg))
    {
    }

    template <typename T>
    bool is() const
    {
        return std::holds_alternative<T>(data_);
    }

    template <>
    bool is<Float>() const
    {
        return std::holds_alternative<Float>(data_) || std::holds_alternative<Integer>(data_);
    }

    template <typename T>
    struct AsReturn {
        using Type = const T&;
    };

    template <>
    struct AsReturn<Float> {
        using Type = Float;
    };

    template <typename T>
    typename AsReturn<T>::Type as() const
    {
        return std::get<T>(data_);
    }

    template <>
    typename AsReturn<Float>::Type as<Float>() const
    {
        if (is<Integer>()) {
            return static_cast<Float>(as<Integer>());
        } else {
            return std::get<Float>(data_);
        }
    }

private:
    Variant data_;
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
    ParseResult(U&& arg)
        : result(std::forward<U>(arg))
    {
    }

    ParseResult(ParseResult&&) = default;

    explicit operator bool() const
    {
        return std::holds_alternative<T>(result);
    }

    const ParseError& error() const
    {
        return std::get<ParseError>(result);
    }

    ParseError& error()
    {
        return std::get<ParseError>(result);
    }

    const T& operator*() const
    {
        return std::get<T>(result);
    }

    T& operator*()
    {
        return std::get<T>(result);
    }
};

std::string getContextString(std::string_view str, const Position& position);

ParseResult<Node::Dictionary> parse(std::string_view str);

} // namespace joml
