#pragma once
#include <string>
#include <variant>

enum class TokenType {
    COMMAND,
    LITERAL,
    QUOTED_STRING
};

class Token {
public:
    TokenType type;
    std::string value;

    Token(TokenType t, std::string v);

    bool isCommand() const;
    bool isLiteral() const;
    bool isQuoted() const;

    std::string unquoted() const;
};