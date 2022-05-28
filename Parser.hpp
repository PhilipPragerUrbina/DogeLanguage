//
// Created by Philip on 5/21/2022.
//

#ifndef PROGRAM_PARSER_HPP
#define PROGRAM_PARSER_HPP

#include "Expression.hpp"


typedef std::vector<Statement*> statementList;

class Parser {
public:
    //create parser from tokens
    Parser(  std::vector<Token> tokens, ErrorHandler* error_handler){
        m_error_handler = error_handler;
        m_tokens = tokens;
    }
    //parse the tokens and return the parent node of the AST
    statementList parse() {
        statementList statements;
        while (m_tokens[m_current].type != END) {
            statements.push_back(declaration());
        }
        return statements;
    }
private:
    ErrorHandler* m_error_handler;
    std::vector<Token> m_tokens;
    //current token
    int m_current = 0;

    //checks
    bool match(std::vector<TokenType> types) {
        for (TokenType type : types) {
            if (check(type)) {m_current++; return true;}
        }
        return false;
    }
    bool check(TokenType type){
        if(m_tokens[m_current].type == END){return false;}
        return m_tokens[m_current].type == type;
    }
    //TODO proper line number errors during runtime
    int getLine(){
        return m_tokens[m_current-1].line;
    }
    Token consume(TokenType type, std::string message) {
        if (check(type)) {m_current++; return m_tokens[m_current-1];}else{
            m_error_handler->error(m_tokens[m_current].line, message);
        }
    }


    //grammar functions

    Statement* declaration(){
        if(match({HASH})){return importDeclaration();}
        if(match({VAR})){return variableDeclaration();}
        if(match({CONST})){   if(match({VAR})){return variableDeclaration(true);}}
        if(match({CLASS})){return classDeclaration();}
        return statement();
    }
    Statement* importDeclaration(){
        if(match({IMPORT})){
            Token name = consume(IDENTIFIER, "Expected import name");
            consume(SEMICOLON, "Expected ; after import");
            return new ImportStatement(name);
        }
        else if(match({INCLUDE})){
            Expression* directory = expression();
            consume(SEMICOLON, "Expected ; after include");
            return new IncludeStatement(directory);
        }
        m_error_handler->error("Expected imports");
        return nullptr;
    }

    Statement* classDeclaration(){
        Token name = consume(IDENTIFIER, "Expected class name");
        consume(LEFT_BRACE, "Expected { after parameters");
        statementList body = block();
        return new ClassStatement(name,body);
    }


    Statement* variableDeclaration(bool constant = false){
        Token name = consume(IDENTIFIER, "Expected variable name");
        if(check(LEFT_PAREN)){return functionDeclaration(name);}

        Expression* initializer = nullptr;
        if(match({EQUAL})){
            initializer = expression();
        }
        consume(SEMICOLON, "Expected ; after expression");
        return new VariableStatement(initializer, name,constant);
    }
    Statement* functionDeclaration(Token name){

        consume(LEFT_PAREN, "Expected ( after function");

        std::vector<Token> parameters;
        if (!check(RIGHT_PAREN)) {
            do {
                parameters.push_back(consume(IDENTIFIER, "Expected parameter name"));
            } while (match({COMMA}));
        }
        consume(RIGHT_PAREN, "Expected ) after parameters");

        consume(LEFT_BRACE, "Expected { after parameters");
        statementList body = block();
        return new FunctionStatement(name, parameters,body);
    };

    Statement* statement(){
        if(match({IF})){return  ifStatement();}
        if(match({RETURN})){return  returnStatement();}
        if(match({BREAK})){return  loopStatement();}
        if(match({CONTINUE})){return  loopStatement();}
        if(match({WHILE})){return  whileStatement();}
        if(match({FOR})){return  forStatement();}
        if (match({LEFT_BRACE})) return new BlockStatement(block());
        return expressionStatement();
    }



    Statement* ifStatement(){
        consume(LEFT_PAREN, "Expected ( after if");
        Expression* condition = expression();
        consume(RIGHT_PAREN, "Expected ) after conditional");

        Statement* then_statement = statement();
        Statement* else_statement = nullptr;
        if(match({ELSE})){
            else_statement = statement();
        }
        return new IfStatement(condition,then_statement,else_statement);
    }
    //is in loop?
    int m_loop = 0;
    Statement* whileStatement(){
        consume(LEFT_PAREN, "Expected ( after if");
        Expression* condition = expression();
        consume(RIGHT_PAREN, "Expected ) after conditional");
        m_loop++;
        Statement* body = statement();
        m_loop--;
        return new WhileStatement(condition,body);
    }
    Statement* forStatement(){
        consume(LEFT_PAREN, "Expected ( after if");
        Statement* initializer = nullptr;
        if(match({VAR})){
            initializer = variableDeclaration();
        }else{
            initializer = expressionStatement();
        }
        Expression* condition = nullptr;
        condition = expression();
        consume(SEMICOLON,"Expected for loop condition ;");
        Expression* increment = nullptr;
        increment = expression();
        consume(RIGHT_PAREN, "Expected ) after for");
        Statement* body = statement();
        body = new BlockStatement({body, new ExpressionStatement(increment)});
        body = new WhileStatement(condition,body);
        body = new BlockStatement({initializer,body});
        return body;
    }

    statementList block(){
        statementList statements;
        while (!check(RIGHT_BRACE) && m_tokens[m_current].type != END){
            statements.push_back(declaration());
        }
        consume(RIGHT_BRACE, "Expected } after block");

        return statements;
    }   //TODO make proper statements for break and continue.
    Statement* loopStatement(){
        if(m_loop == 0){
            m_error_handler->error(m_tokens[m_current-1].line, "Break called not in loop");
        }
        Token keyword = m_tokens[m_current-1];
        consume(SEMICOLON, "Expected ; after expression");
        return new ReturnStatement(keyword, nullptr);
    }
    Statement* returnStatement(){
        Token keyword = m_tokens[m_current-1];
        Expression* right = nullptr;
        if(!check(SEMICOLON)){
            right = expression();
        }
        consume(SEMICOLON, "Expected ; after expression");
        return new ReturnStatement(keyword, right);
    }


    Statement* expressionStatement(){
        Expression* right = expression();
        consume(SEMICOLON, "Expected ; after expression");
        return new ExpressionStatement(right);
    }

    Expression* expression(){
        return assignment();
    }

    Expression* assignment(){
        Expression* left = or_();
        if(match({EQUAL})){
            Token equals = m_tokens[m_current-1];
            Expression* right = assignment();
            if (Variable* variable = dynamic_cast<Variable*>(left)){

                return new Assign(right,variable->m_name);
            }
            else if (Pointer* ptr = dynamic_cast<Pointer*>(left)){

                return new Assign(right,ptr->m_variable->m_name, true);
            }
            else if (Get* obj = dynamic_cast<Get*>(left)){

                return new Set(right,obj->m_name, obj->m_object);
            }
            m_error_handler->error(m_tokens[m_current-1].line, "Assignment error");
        }
        return left;
    }

    Expression* or_(){
        Expression* left = and_();
        while (match({OR})){
            Token operation_ =   m_tokens[m_current-1];
            Expression* right = and_();
            left= new Logic(left, operation_, right);
        }
        return left;
    }

    Expression* and_(){
        Expression* left = equality();
        while (match({AND})){
            Token operation_ =   m_tokens[m_current-1];
            Expression* right = equality();
            left= new Logic(left, operation_, right);
        }
        return left;
    }

    Expression* equality(){
        Expression* left= comparison();
        while (match({BANG_EQUAL, EQUAL_EQUAL})) {
            Token operation_ =  m_tokens[m_current-1];
            Expression* right = comparison();
            left= new Binary(left, operation_, right);
        }
        return left;
    }
 Expression* comparison() {
     Expression* left= term();
        while (match({GREATER, GREATER_EQUAL, LESS, LESS_EQUAL})) {
            Token operation_ =  m_tokens[m_current-1];
            Expression* right = term();
            left= new Binary(left, operation_, right);
        }
        return left;
    }
 Expression* term() {
        Expression* left= factor();
        while (match({MINUS, PLUS})) {
            Token operation_ = m_tokens[m_current-1];
            Expression* right = factor();
            left= new Binary(left, operation_, right);
        }
        return left;
    }
    Expression*  factor() {
        Expression*  left= unary();
        while (match({SLASH, STAR})) {
            Token operation_  = m_tokens[m_current-1];
            Expression* right = unary();
            left= new Binary(left, operation_, right);
        }
        return left;
    }
    Expression* unary() {
        if (match({BANG, MINUS})) {
            Token operation_ = m_tokens[m_current-1];
            Expression* right = unary();
            return new Unary(operation_, right);
        }
        return callExpression();
    }
    Expression* callExpression(){
        Expression* left = primary();
        while(true){
            if(match({LEFT_PAREN})){
            left = callFinalize(left);}
            else if(match({DOT})){
                Token name = consume(IDENTIFIER,
                                     "Expected property name");
                left = new Get(left,name);
            }else{
                break;
            }
        }
        return left;
    }
    Expression* callFinalize(Expression* callee){
        std::vector<Expression*> arguments;
        if (!check(RIGHT_PAREN)) {
            do {
                arguments.push_back(expression());
            } while (match({COMMA}));
        }
        Token paren = consume(RIGHT_PAREN,
                              "Expect ')' after arguments.");

        return  new Call(callee, paren, arguments);
    }
    Expression* primary() {
        if (match({FALSE})) return new Literal(false);
        if (match({TRUE})) return new Literal(true);
        if (match({NIL})) return new Literal(null_object());
        if (match({FLOATING,INTEGER, STRING})) {
            return new Literal(m_tokens[m_current-1].value);
        }
        if(match({REFERENCE})){
            if(match({IDENTIFIER})){
                return new Pointer(new Variable(m_tokens[m_current-1]));
            }
        }

        if (match({IDENTIFIER})) {
            return new Variable(m_tokens[m_current-1]);
        }
        if (match({LEFT_PAREN})) {
            Expression* middle = expression();
            consume(RIGHT_PAREN, "Expected ) after expression: " + m_tokens[m_current-1].original); //after expression check for right parentheses
            return new Grouping(middle);
        }
        m_error_handler->error(m_tokens[m_current].line, "expected expression: " + m_tokens[m_current].original);
        return NULL;
    }
};

#endif PROGRAM_PARSER_HPP