#include "../include/Token.hpp"
#include <sstream>

std::string Token::asString() const {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    }
    return "";
}

double Token::asNumber() const {
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    return 0.0;
}

bool Token::is(const TokenType& t) const { 
    return type == t; 
}

bool Token::isCommand() const { 
    return type == TokenType::COMMAND; 
}

bool Token::isQuoted() const { 
    return type == TokenType::STRING; 
}

std::string Token::unquoted() const { 
    return asString(); 
}