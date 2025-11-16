#pragma once
#include <string>
#include <variant>

enum class TokenType { COMMAND, IDENTIFIER, NUMBER, STRING, END };

struct Token {
    TokenType type;
    std::variant<std::monostate, std::string, double> value;

    Token(TokenType t) : type(t) {}
    Token(TokenType t, std::string v) : type(t), value(std::move(v)) {}
    Token(TokenType t, double v) : type(t), value(v) {}

    std::string asString() const;
    double asNumber() const;
    bool is(const TokenType& t) const;
    bool isCommand() const;
    bool isQuoted() const;
    std::string unquoted() const;
};