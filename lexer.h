#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "token.h"

class Lexer {
private:
    std::string source;
    std::size_t pos;
    int line;
    int column;

    std::unordered_map<std::string, TokenType> keywords;

    bool isAtEnd() const;
    char peek(int lookahead = 0) const;
    char advance();
    bool match(char expected);

    Token makeToken(TokenType type, const std::string& lexeme, int startLine, int startColumn) const;
    Token errorToken(const std::string& message, int errLine, int errColumn) const;

    void skipWhitespaceAndComments();

    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanString();
    Token scanOperatorOrDelimiter();

    void initKeywords();

public:
    explicit Lexer(const std::string& source);

    Token nextToken();
    std::vector<Token> tokenizeAll();
};
