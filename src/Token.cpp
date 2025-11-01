#include "../include/Token.hpp"
#include <cctype>

Token::Token(TokenType t, std::string v) : type(t), value(std::move(v)) {

}

bool Token::isCommand() const { 
    return type == TokenType::COMMAND; 
}
 
bool Token::isLiteral() const { 
    return type == TokenType::LITERAL; 
}

bool Token::isQuoted() const { 
    return type == TokenType::QUOTED_STRING; 
}

std::string Token::unquoted() const {
    if (type != TokenType::QUOTED_STRING) {
        return value;
    }
    std::string s = value.substr(1, value.size() - 2);
    return s;
}