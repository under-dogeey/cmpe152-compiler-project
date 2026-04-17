#include "lexer.h"

#include <cctype>

namespace {

bool isIdentStart(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isIdentCont(char c)
{
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

} // namespace

Lexer::Lexer(const std::string& source)
    : source(source)
    , pos(0)
    , line(1)
    , column(1)
{
    initKeywords();
}

void Lexer::initKeywords()
{
    keywords["print"] = TokenType::PRINT;
}

bool Lexer::isAtEnd() const
{
    return pos >= source.size();
}

char Lexer::peek(int lookahead) const
{
    const std::size_t i = pos + static_cast<std::size_t>(lookahead);
    if (i >= source.size()) {
        return '\0';
    }
    return source[i];
}

char Lexer::advance()
{
    if (isAtEnd()) {
        return '\0';
    }
    const char c = source[pos++];
    if (c == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    return c;
}

bool Lexer::match(char expected)
{
    if (isAtEnd() || peek() != expected) {
        return false;
    }
    advance();
    return true;
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme, int startLine, int startColumn) const
{
    return Token(type, lexeme, startLine, startColumn);
}

Token Lexer::errorToken(const std::string& message, int errLine, int errColumn) const
{
    return Token(TokenType::ERROR, message, errLine, errColumn);
}

void Lexer::skipWhitespaceAndComments()
{
    while (!isAtEnd()) {
        const char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '#') {
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::scanIdentifierOrKeyword()
{
    const int startLine = line;
    const int startColumn = column;
    std::string lex;
    while (!isAtEnd() && isIdentCont(peek())) {
        lex.push_back(advance());
    }
    const auto it = keywords.find(lex);
    if (it != keywords.end()) {
        return makeToken(it->second, lex, startLine, startColumn);
    }
    return makeToken(TokenType::IDENTIFIER, lex, startLine, startColumn);
}

Token Lexer::scanNumber()
{
    const int startLine = line;
    const int startColumn = column;
    std::string lex;
    if (!isDigit(peek())) {
        return errorToken("Expected digit", startLine, startColumn);
    }
    while (!isAtEnd() && isDigit(peek())) {
        lex.push_back(advance());
    }
    return makeToken(TokenType::NUMBER, lex, startLine, startColumn);
}

Token Lexer::scanString()
{
    const int startLine = line;
    const int startColumn = column;
    const char quote = peek();
    if (quote != '\'' && quote != '"') {
        return errorToken("Expected string delimiter", startLine, startColumn);
    }
    advance(); // opening quote

    std::string inner;
    while (!isAtEnd() && peek() != quote) {
        if (peek() == '\n') {
            return errorToken("Unterminated string literal", startLine, startColumn);
        }
        if (peek() == '\\') {
            advance();
            if (isAtEnd()) {
                return errorToken("Unterminated string literal", startLine, startColumn);
            }
            const char esc = advance();
            switch (esc) {
            case 'n':
                inner.push_back('\n');
                break;
            case 't':
                inner.push_back('\t');
                break;
            case '\\':
                inner.push_back('\\');
                break;
            case '"':
                inner.push_back('"');
                break;
            case '\'':
                inner.push_back('\'');
                break;
            default:
                inner.push_back(esc);
                break;
            }
        } else {
            inner.push_back(advance());
        }
    }
    if (peek() != quote) {
        return errorToken("Unterminated string literal", startLine, startColumn);
    }
    advance(); // closing quote
    return makeToken(TokenType::STRING, inner, startLine, startColumn);
}

Token Lexer::scanOperatorOrDelimiter()
{
    const int startLine = line;
    const int startColumn = column;
    const char c = peek();
    switch (c) {
    case '+':
        advance();
        return makeToken(TokenType::PLUS, "+", startLine, startColumn);
    case '-':
        advance();
        return makeToken(TokenType::MINUS, "-", startLine, startColumn);
    case '*':
        advance();
        return makeToken(TokenType::STAR, "*", startLine, startColumn);
    case '/':
        advance();
        return makeToken(TokenType::SLASH, "/", startLine, startColumn);
    case '%':
        advance();
        return makeToken(TokenType::PERCENT, "%", startLine, startColumn);
    case '=':
        advance();
        return makeToken(TokenType::ASSIGN, "=", startLine, startColumn);
    case '(':
        advance();
        return makeToken(TokenType::LPAREN, "(", startLine, startColumn);
    case ')':
        advance();
        return makeToken(TokenType::RPAREN, ")", startLine, startColumn);
    case ',':
        advance();
        return makeToken(TokenType::COMMA, ",", startLine, startColumn);
    default:
        return errorToken(std::string("Unexpected character '") + c + "'", startLine, startColumn);
    }
}

Token Lexer::nextToken()
{
    skipWhitespaceAndComments();

    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, "", line, column);
    }

    const char c = peek();
    const int startLine = line;
    const int startColumn = column;

    if (c == '\n') {
        advance();
        return makeToken(TokenType::NEWLINE, std::string(1, '\n'), startLine, startColumn);
    }

    if (isIdentStart(c)) {
        return scanIdentifierOrKeyword();
    }
    if (isDigit(c)) {
        Token t = scanNumber();
        if (t.type == TokenType::ERROR && !isAtEnd()) {
            advance();
        }
        return t;
    }
    if (c == '\'' || c == '"') {
        Token t = scanString();
        if (t.type == TokenType::ERROR && !isAtEnd()) {
            advance();
        }
        return t;
    }

    Token t = scanOperatorOrDelimiter();
    if (t.type == TokenType::ERROR && !isAtEnd()) {
        advance();
    }
    return t;
}

std::vector<Token> Lexer::tokenizeAll()
{
    std::vector<Token> tokens;
    while (true) {
        Token t = nextToken();
        if (t.type == TokenType::END_OF_FILE) {
            break;
        }
        tokens.push_back(std::move(t));
    }
    return tokens;
}
