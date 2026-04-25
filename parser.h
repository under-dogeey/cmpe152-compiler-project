#pragma once

#include <memory>
#include <string>
#include <vector>

#include "token.h"

struct ParseError {
    int line;
    int column;
    std::string type;
    std::string message;
};

struct Expr {
    virtual ~Expr() {}
    virtual std::string toCpp() const = 0;
};

struct NumberExpr : Expr {
    std::string value;

    explicit NumberExpr(const std::string& value)
        : value(value) {}

    std::string toCpp() const override;
};

struct StringExpr : Expr {
    std::string value;

    explicit StringExpr(const std::string& value)
        : value(value) {}

    std::string toCpp() const override;
};

struct IdentifierExpr : Expr {
    std::string name;

    explicit IdentifierExpr(const std::string& name)
        : name(name) {}

    std::string toCpp() const override;
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left;
    TokenType op;
    std::unique_ptr<Expr> right;

    BinaryExpr(std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
        : left(std::move(left))
        , op(op)
        , right(std::move(right)) {}

    std::string toCpp() const override;
};

struct Stmt {
    virtual ~Stmt() {}
    virtual std::string toCpp() const = 0;
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    AssignStmt(const std::string& name, std::unique_ptr<Expr> value)
        : name(name)
        , value(std::move(value)) {}

    std::string toCpp() const override;
};

struct PrintStmt : Stmt {
    std::vector<std::unique_ptr<Expr> > arguments;

    std::string toCpp() const override;
};

struct Program {
    std::vector<std::unique_ptr<Stmt> > statements;
};

class Parser {
private:
    std::vector<Token> tokens;
    std::size_t current;
    std::vector<ParseError> errors;

    bool isAtEnd() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool matchAny(const std::vector<TokenType>& types);

    void addError(const Token& token, const std::string& type, const std::string& message);
    void synchronize();
    void skipNewlines();

    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseAssignment();
    std::unique_ptr<Stmt> parsePrintStatement();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseTerm();
    std::unique_ptr<Expr> parseFactor();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();

public:
    explicit Parser(const std::vector<Token>& tokens);

    Program parseProgram();
    const std::vector<ParseError>& getErrors() const;
};

std::string generateCppProgram(const Program& program);
