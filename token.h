#pragma once

#include <string>

enum class TokenType {
    IDENTIFIER,
    NUMBER,
    STRING,
    PRINT,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    ASSIGN, // =
    LPAREN,
    RPAREN,
    COMMA,
    NEWLINE,
    END_OF_FILE,
    ERROR
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    Token(TokenType type, const std::string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column) {}
};
