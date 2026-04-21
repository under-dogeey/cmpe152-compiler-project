#include "parser.h"

#include <sstream>

namespace {

std::string tokenOpToString(TokenType type)
{
    switch (type) {
    case TokenType::PLUS:
        return "+";
    case TokenType::MINUS:
        return "-";
    case TokenType::STAR:
        return "*";
    case TokenType::SLASH:
        return "/";
    case TokenType::PERCENT:
        return "%";
    default:
        return "?";
    }
}

std::string escapeCppString(const std::string& s)
{
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

} // namespace

std::string NumberExpr::toCpp() const
{
    return value;
}

std::string StringExpr::toCpp() const
{
    return std::string("\"") + escapeCppString(value) + "\"";
}

std::string IdentifierExpr::toCpp() const
{
    return name;
}

std::string BinaryExpr::toCpp() const
{
    return "(" + left->toCpp() + " " + tokenOpToString(op) + " " + right->toCpp() + ")";
}

std::string AssignStmt::toCpp() const
{
    return "auto " + name + " = " + value->toCpp() + ";";
}

std::string PrintStmt::toCpp() const
{
    std::string line = "std::cout";
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        line += " << ";
        line += arguments[i]->toCpp();
        if (i + 1 < arguments.size()) {
            line += " << \" \"";
        }
    }
    line += " << std::endl;";
    return line;
}

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens)
    , current(0)
{
}

bool Parser::isAtEnd() const
{
    return current >= tokens.size();
}

const Token& Parser::peek() const
{
    return tokens[current];
}

const Token& Parser::previous() const
{
    return tokens[current - 1];
}

const Token& Parser::advance()
{
    if (!isAtEnd()) {
        ++current;
    }
    return previous();
}

bool Parser::check(TokenType type) const
{
    if (isAtEnd()) {
        return false;
    }
    return peek().type == type;
}

bool Parser::match(TokenType type)
{
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchAny(const std::vector<TokenType>& types)
{
    for (std::size_t i = 0; i < types.size(); ++i) {
        if (check(types[i])) {
            advance();
            return true;
        }
    }
    return false;
}

void Parser::addError(const Token& token, const std::string& message)
{
    ParseError error;
    error.line = token.line;
    error.column = token.column;
    error.message = message;
    errors.push_back(error);
}

void Parser::synchronize()
{
    while (!isAtEnd()) {
        if (check(TokenType::NEWLINE)) {
            advance();
            return;
        }
        advance();
    }
}

void Parser::skipNewlines()
{
    while (match(TokenType::NEWLINE)) {
    }
}

Program Parser::parseProgram()
{
    Program program;
    skipNewlines();

    while (!isAtEnd()) {
        if (check(TokenType::ERROR)) {
            addError(peek(), peek().lexeme);
            advance();
            synchronize();
            skipNewlines();
            continue;
        }

        std::unique_ptr<Stmt> stmt = parseStatement();
        if (stmt) {
            program.statements.push_back(std::move(stmt));
        }

        if (match(TokenType::NEWLINE)) {
            skipNewlines();
        } else if (!isAtEnd()) {
            addError(peek(), "Expected end of line after statement");
            synchronize();
            skipNewlines();
        }
    }

    return program;
}

std::unique_ptr<Stmt> Parser::parseStatement()
{
    if (check(TokenType::PRINT)) {
        return parsePrintStatement();
    }
    if (check(TokenType::IDENTIFIER)) {
        return parseAssignment();
    }

    addError(peek(), "Expected a statement");
    synchronize();
    return std::unique_ptr<Stmt>();
}

std::unique_ptr<Stmt> Parser::parseAssignment()
{
    Token name = advance();

    if (!match(TokenType::ASSIGN)) {
        addError(peek(), "Expected '=' after identifier");
        synchronize();
        return std::unique_ptr<Stmt>();
    }

    std::unique_ptr<Expr> value = parseExpression();
    if (!value) {
        return std::unique_ptr<Stmt>();
    }

    return std::unique_ptr<Stmt>(new AssignStmt(name.lexeme, std::move(value)));
}

std::unique_ptr<Stmt> Parser::parsePrintStatement()
{
    advance();

    if (!match(TokenType::LPAREN)) {
        addError(peek(), "Expected '(' after print");
        synchronize();
        return std::unique_ptr<Stmt>();
    }

    std::unique_ptr<PrintStmt> stmt(new PrintStmt());

    if (match(TokenType::RPAREN)) {
        return std::unique_ptr<Stmt>(stmt.release());
    }

    do {
        std::unique_ptr<Expr> expr = parseExpression();
        if (!expr) {
            synchronize();
            return std::unique_ptr<Stmt>();
        }
        stmt->arguments.push_back(std::move(expr));
    } while (match(TokenType::COMMA));

    if (!match(TokenType::RPAREN)) {
        addError(peek(), "Expected ')' after print arguments");
        synchronize();
        return std::unique_ptr<Stmt>();
    }

    return std::unique_ptr<Stmt>(stmt.release());
}

std::unique_ptr<Expr> Parser::parseExpression()
{
    std::unique_ptr<Expr> expr = parseTerm();

    while (matchAny(std::vector<TokenType>{ TokenType::PLUS, TokenType::MINUS })) {
        TokenType op = previous().type;
        std::unique_ptr<Expr> right = parseTerm();
        if (!right) {
            return std::unique_ptr<Expr>();
        }
        expr.reset(new BinaryExpr(std::move(expr), op, std::move(right)));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm()
{
    std::unique_ptr<Expr> expr = parseFactor();

    while (matchAny(std::vector<TokenType>{ TokenType::STAR, TokenType::SLASH, TokenType::PERCENT })) {
        TokenType op = previous().type;
        std::unique_ptr<Expr> right = parseFactor();
        if (!right) {
            return std::unique_ptr<Expr>();
        }
        expr.reset(new BinaryExpr(std::move(expr), op, std::move(right)));
    }

    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor()
{
    return parseUnary();
}

std::unique_ptr<Expr> Parser::parseUnary()
{
    if (match(TokenType::MINUS)) {
        std::unique_ptr<Expr> right = parseUnary();
        if (!right) {
            return std::unique_ptr<Expr>();
        }
        std::unique_ptr<Expr> zero(new NumberExpr("0"));
        return std::unique_ptr<Expr>(new BinaryExpr(std::move(zero), TokenType::MINUS, std::move(right)));
    }

    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary()
{
    if (match(TokenType::NUMBER)) {
        return std::unique_ptr<Expr>(new NumberExpr(previous().lexeme));
    }

    if (match(TokenType::STRING)) {
        return std::unique_ptr<Expr>(new StringExpr(previous().lexeme));
    }

    if (match(TokenType::IDENTIFIER)) {
        return std::unique_ptr<Expr>(new IdentifierExpr(previous().lexeme));
    }

    if (match(TokenType::LPAREN)) {
        std::unique_ptr<Expr> expr = parseExpression();
        if (!expr) {
            return std::unique_ptr<Expr>();
        }
        if (!match(TokenType::RPAREN)) {
            addError(peek(), "Expected ')' after expression");
            return std::unique_ptr<Expr>();
        }
        return expr;
    }

    if (!isAtEnd()) {
        addError(peek(), "Expected expression");
    }
    return std::unique_ptr<Expr>();
}

const std::vector<ParseError>& Parser::getErrors() const
{
    return errors;
}

std::string generateCppProgram(const Program& program)
{
    std::ostringstream out;

    out << "#include <iostream>\n";
    out << "#include <string>\n\n";
    out << "int main()\n";
    out << "{\n";

    for (std::size_t i = 0; i < program.statements.size(); ++i) {
        out << "    " << program.statements[i]->toCpp() << "\n";
    }

    out << "    return 0;\n";
    out << "}\n";

    return out.str();
}
