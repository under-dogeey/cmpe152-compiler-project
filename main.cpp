#include <iostream>
#include <string>
#include <vector>

#include "lexer.h"
#include "parser.h"

static std::string tokenTypeName(TokenType type)
{
    switch (type) {
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::NUMBER:
        return "NUMBER";
    case TokenType::STRING:
        return "STRING";
    case TokenType::PRINT:
        return "PRINT";
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::STAR:
        return "STAR";
    case TokenType::SLASH:
        return "SLASH";
    case TokenType::PERCENT:
        return "PERCENT";
    case TokenType::ASSIGN:
        return "ASSIGN";
    case TokenType::LPAREN:
        return "LPAREN";
    case TokenType::RPAREN:
        return "RPAREN";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::NEWLINE:
        return "NEWLINE";
    case TokenType::END_OF_FILE:
        return "END_OF_FILE";
    case TokenType::ERROR:
        return "ERROR";
    }
    return "UNKNOWN";
}

static void printTokens(const std::vector<Token>& tokens)
{
    for (const auto& token : tokens) {
        std::cout << "Type: " << tokenTypeName(token.type) << ", Lexeme: ";
        if (token.type == TokenType::NEWLINE) {
            std::cout << "\\n";
        } else {
            std::cout << token.lexeme;
        }
        std::cout << " (line " << token.line << ", col " << token.column << ")\n";
    }
}

int main()
{

    const std::string sourceCode =
        "a = 10\n"
        "b = 3\n"
        "c = a + b * 2\n"
        "msg = \"sum=\"\n"
        "print(msg, c)\n";

    Lexer lexer(sourceCode);

    std::cout << "Source code:\n" << sourceCode << "\n";
    std::cout << "Tokens from lexical analyzer:\n";
    const std::vector<Token> tokens = lexer.tokenizeAll();
    printTokens(tokens);

    Parser parser(tokens);

    Program program = parser.parseProgram();

    if (!parser.getErrors().empty())
    {
    std::cout << "Syntax Errors:\n";

    const std::vector<ParseError>& errors = parser.getErrors();
    for (std::size_t i = 0; i < errors.size(); i++) {
        std::cout << "Line " << errors[i].line
                  << ", Column " << errors[i].column
                  << ": " << errors[i].message << "\n";
        }
    }
    else
    {
    std::cout << "Generated C++ Code:\n";
    std::cout << generateCppProgram(program) << std::endl;
    }


    return 0;
}
