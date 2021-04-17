#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/*extern "C" {
typedef struct joml_file joml_file;
typedef struct joml_node joml_node;

typedef enum {
    joml_node_type_string,
    joml_node_type_integer,
    joml_node_type_float,
    joml_node_type_array,
    joml_node_type_dictionary,
} joml_node_type;

void joml_parse(joml_file* file, const char* buffer, size_t size);
const joml_node* joml_root_node(const joml_file* file);
joml_node_type joml_node_type(const joml_node* node);
const char* joml_string(const joml_node* node);
int64_t joml_integer(const joml_node* node);
double joml_float(const joml_node* node);
size_t joml_array_size(const joml_node* node);
}*/

namespace joml {

class Node {
public:
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
    std::variant<String, Bool, Integer, Float, Array, Dictionary> data_;
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
            NoSeparator,
            ExpectedDictClose,
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
