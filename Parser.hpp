//
// Created by Philip on 5/21/2022.
//

#ifndef PROGRAM_PARSER_HPP
#define PROGRAM_PARSER_HPP

#include <map>
#include "Expression.hpp"

//convenience type
typedef std::vector<Statement*> statementList;

class Parser {
public:
    std::string link_directory = "";
    //create parser from tokens
    Parser(  std::vector<Token> tokens, ErrorHandler* error_handler, std::string file, std::map<std::string ,std::vector<Token >>* templates,
    std::vector<ClassStatement*>* templates_to_resolve){
        m_error_handler = error_handler;
        m_error_handler->m_name = "Parsing";
        m_error_handler->m_file = file;
        m_tokens = tokens;
      m_templates_to_resolve=  templates_to_resolve;
      m_templates = templates;
    }
    Parser(std::map<std::string ,std::vector<Token >>* templates,
           std::vector<ClassStatement*> templates_to_resolve){
        m_templates = templates;
        for(ClassStatement* statement : templates_to_resolve){
            //check if template exists
            if(m_templates->find(statement->m_template_name)!= m_templates->end()){
                //set position
                int last_position = m_current;
                std::vector<Token > last_tokens = m_tokens;
                m_current = 0;
                m_tokens =  m_templates->at(statement->m_template_name);
                //set names
                m_in_class = statement->m_name.original;
                m_template_type = statement->m_template_type;
                m_template_class_name =statement->m_name.original;
                m_template_name = statement->m_template_name;
                //create class
                statementList body = block();
                //revert
                m_in_class = "";
                m_template_type = "";
                m_current = last_position;
                m_template_class_name = "";
                m_template_name = "";
                m_tokens = last_tokens;
                statement->m_members = body;
            }else{
                m_error_handler->error(statement->m_line,"template not found: " + statement->m_template_name);
                return;
            }
        }
    }

    //parse the tokens and return the parent node of the AST
    statementList parse() {
        statementList statements;
        while (m_tokens[m_current].type != END) {
            statements.push_back(declaration());
        }
        return statements;
    }
    std::vector<std::string> m_imports;
    std::vector<std::string> m_includes;
private:
    //for handling errors
    ErrorHandler* m_error_handler;
    std::vector<Token> m_tokens;



    //current token
    int m_current = 0;
    //current class
    std::string m_in_class = "";

    //checks
    //see if current token is of a list of type and go to next if it is
    bool match(std::vector<TokenType> types) {
        for (TokenType type : types) {
            if (check(type)) {m_current++; return true;}
        }
        return false;
    }
    //just see if current token of type
    bool check(TokenType type){
        //template replace type
        if(m_tokens[m_current].original == "type" && m_template_type != ""){
            m_tokens[m_current].original = m_template_type;
            m_tokens[m_current].value = m_template_type;
        }
        if(m_tokens[m_current].original == "type_ptr" && m_template_type != ""){
            m_tokens[m_current].original = m_template_type + "_ptr";
            m_tokens[m_current].value = m_template_type + "_ptr";
        }

        //template replace class name, for constructor
        if(m_tokens[m_current].original == m_template_name && m_template_class_name != ""){
            m_tokens[m_current].original = m_template_class_name;
            m_tokens[m_current].value = m_template_class_name;
        }

        if(m_tokens[m_current].type == END){return false;}
        return m_tokens[m_current].type == type;
    }
    //get the line number
    int getLine(){
        return m_tokens[m_current-1].line;
    }
    //match token and create error otherwise
    Token consume(TokenType type, std::string message) {
        if (check(type)) {m_current++; return m_tokens[m_current-1];}else{
            m_error_handler->error(getLine(), message);
        }
    }
    bool checkClass(){
        if( m_tokens[m_current].type == IDENTIFIER && m_tokens[m_current+1].type == IDENTIFIER ) {
            if (match({IDENTIFIER})) {
                return true;
            }
        }
        return false;
    }

    //grammar functions

    //first declarations
    Statement* declaration(){
        if(match({HASH})){return importDeclaration();}
        if(match({VAR}) || checkClass()){return variableDeclaration(false, m_tokens[m_current-1]);}

        if(match({EXTERN})){   if(match({VAR})){return externDeclaration(m_tokens[m_current-1]);}}
        if(match({CONST})){   if(match({VAR}) || checkClass()){return variableDeclaration(true,m_tokens[m_current-1]);}}
        if(match({CLASS})){
            return classDeclaration();
        }
        if(match({USE})){return templateStatement();}
        return statement();
    }


    Statement* externDeclaration(Token type){
        Token name = consume(IDENTIFIER, "Expected extern m_visitor_name.");
        consume(LEFT_PAREN, "Expected ( after extern m_visitor_name.");
        std::vector<VariableStatement*> parameters;
        if (!check(RIGHT_PAREN)) {
            do {
                Token param_type = consume(VAR, "Expected parameter type");
                Token param_name = consume(IDENTIFIER, "Expected parameter name");

                parameters.push_back(new VariableStatement(nullptr,param_name,false,param_type,getLine()));
            } while (match({COMMA}));
        }
        consume(RIGHT_PAREN, "Expected ) after parameters.");
        consume(SEMICOLON, "Expected ; after extern.");
        return new FunctionStatement(name, parameters, {}, type, getLine(), m_in_class);
    }

    //#import SL; or #include "/new.doge";
    Statement* importDeclaration(){
        if(match({IMPORT})){
            Token name = consume(IDENTIFIER, "Expected import m_visitor_name.");
            consume(SEMICOLON, "Expected ; after import.");
            m_imports.push_back(name.original);
            return new ImportStatement(name, getLine());
        }
        else if(match({INCLUDE})){
            Token directory = consume(STRING, "Expected include directory.");
            consume(SEMICOLON, "Expected ; after include.");
            m_includes.push_back(std::get<std::string>(directory.value));
            return new IncludeStatement(directory, getLine(),false);
        }  else if(match({LINK})){
            Token directory = consume(STRING, "Expected link directory.");
            consume(SEMICOLON, "Expected ; after include.");
            if(link_directory != ""){
                directory .value = link_directory + "/" + std::get<std::string>(directory.value);
            }
            return new IncludeStatement(directory, getLine(), true);
        }
        m_error_handler->error("Expected imports or includes.");
        return nullptr;
    }
    //keep track of templates
    std::map<std::string ,std::vector<Token >>* m_templates;
    std::vector<ClassStatement*>* m_templates_to_resolve;

    //substitute type
    std::string m_template_type = "";
    //substitute class name
    std::string m_template_class_name = "";
    //original template name
    std::string m_template_name = "";


    Statement* templateStatement(){
        //get info
        Token template_name = consume(IDENTIFIER, "Expected template name.");
        consume(USE, "Expected as after template name.");
        Token class_name =  consume(IDENTIFIER, "Expected class name.");
        consume(USE, "Expected using after class name.");
        match({VAR, IDENTIFIER});
        Token type_name =  m_tokens[m_current-1];
        consume(SEMICOLON, "Expected ; after template statement.");
        //check if template exists
        if(m_templates->find(template_name.original)!= m_templates->end()){
            //set position
            int last_position = m_current;
            std::vector<Token > last_tokens = m_tokens;
            m_current = 0;
            m_tokens =  m_templates->at(template_name.original);
            //set names
            m_in_class = class_name.original;
            m_template_type = type_name.original;
           m_template_class_name =class_name.original;
            m_template_name = template_name.original;
            //create class
            statementList body = block();
            //revert
            m_in_class = "";
            m_template_type = "";
            m_current = last_position;
            m_template_class_name = "";
            m_template_name = "";
            m_tokens = last_tokens;
            //return new template class
            return new ClassStatement(class_name, body, getLine());
        }else{
            //resolve later, may not be in same file
            ClassStatement* to_return = new ClassStatement(class_name,{},getLine(), template_name.original, type_name.original);
            m_templates_to_resolve->push_back(to_return);
            return  to_return;
        }


    }

    //class cake{}
    Statement* classDeclaration(){
    bool is_template = match({TEMPLATE});

        Token name = consume(IDENTIFIER, "Expected class m_visitor_name.");
        consume(LEFT_BRACE, "Expected { class m_visitor_name.");
        //if template just save position, go through the body and return
        if(is_template){
            std::vector<Token>::const_iterator first = m_tokens.begin() + m_current;


            m_in_class = name.original;
            block();
            m_in_class = "";
            std::vector<Token>::const_iterator last = m_tokens.begin() + m_current;
            m_templates->insert_or_assign(name.original,std::vector<Token>(first,last));
            return new EmptyStatement();
        }
        m_in_class = name.original;
        statementList body = block();
        m_in_class = "";

        return new ClassStatement(name,body, getLine());
    }

    //var foo = bar; or var foo;
    Statement* variableDeclaration(bool constant,Token type){
        Token name = consume(IDENTIFIER, "Expected variable m_visitor_name.");
        if(check(LEFT_PAREN)){return functionDeclaration(name,type);}

        Expression* initializer = nullptr;
        if(match({EQUAL})){
            initializer = expression();
        }
        consume(SEMICOLON, "Expected ; after expression.");
        return new VariableStatement(initializer, name,constant,type, getLine());
    }
    //var a(b,c){}
    Statement* functionDeclaration(Token name, Token type){
        consume(LEFT_PAREN, "Expected ( after function m_visitor_name.");
        std::vector<VariableStatement*> parameters;
        if (!check(RIGHT_PAREN)) {
            do {
                match({IDENTIFIER, VAR});
                Token param_type = m_tokens[m_current-1];
                Token param_name = consume(IDENTIFIER, "Expected parameter name");

                parameters.push_back(new VariableStatement(nullptr,param_name,false,param_type,getLine()));
            } while (match({COMMA}));
        }
        consume(RIGHT_PAREN, "Expected ) after parameters.");
        consume(LEFT_BRACE, "Expected { after parameters.");
        statementList body = block();
        return new FunctionStatement(name, parameters, body, type, getLine(), m_in_class);
    };

    //next match statements
    Statement* statement(){
        if(match({IF})){return  ifStatement();}
        if(match({RETURN})){return  returnStatement();}
        if(match({BREAK})){return  loopStatement();}
        if(match({WHILE})){return  whileStatement();}
        if(match({FOR})){return  forStatement();}
        if (match({LEFT_BRACE})) return new BlockStatement(block(), getLine());
        return expressionStatement();
    }

    //if(a == true){}
    Statement* ifStatement(){
        consume(LEFT_PAREN, "Expected ( after if.");
        Expression* condition = expression();
        consume(RIGHT_PAREN, "Expected ) after conditions.");
        Statement* then_statement = statement();
        Statement* else_statement = nullptr;
        if(match({ELSE})){
            else_statement = statement();
        }
        return new IfStatement(condition,then_statement,else_statement, getLine());
    }

    //while(true){}
    Statement* whileStatement(){
        consume(LEFT_PAREN, "Expected ( after if");
        Expression* condition = expression();
        consume(RIGHT_PAREN, "Expected ) after conditional");
        Statement* body = statement();
        return new WhileStatement(condition,body, getLine());
    }

    //Turn for statement into while statement
    Statement* forStatement(){
        consume(LEFT_PAREN, "Expected ( after for.");
        Statement* initializer = nullptr;
        if(match({VAR})){
            initializer = variableDeclaration(false,m_tokens[m_current-1]);
        }else{
            initializer = expressionStatement();
        }
        Expression* condition = expression();
        consume(SEMICOLON,"Expected ; after condition.");
        Expression* increment = expression();
        consume(RIGHT_PAREN, "Expected ) after for.");
        Statement* body = statement();
        body = new BlockStatement({body, new ExpressionStatement(increment, getLine())}, getLine());
        body = new WhileStatement(condition,body, getLine());
        body = new BlockStatement({initializer,body}, getLine());
        return body;
    }

    //{}
    statementList block(){
        statementList statements;
        while (!check(RIGHT_BRACE) && m_tokens[m_current].type != END){
            statements.push_back(declaration());
        }
        consume(RIGHT_BRACE, "Expected } after block.");

        return statements;
    }

    Statement* loopStatement(){
        Token keyword = m_tokens[m_current-1];
        consume(SEMICOLON, "Expected ; after expression.");
        return new ReturnStatement(keyword, nullptr, getLine());
    }
    Statement* returnStatement(){
        Token keyword = m_tokens[m_current-1];
        Expression* right = nullptr;
        if(!check(SEMICOLON)){
            right = expression();
        }
        consume(SEMICOLON, "Expected ; after expression.");
        return new ReturnStatement(keyword, right, getLine());
    }


    Statement* expressionStatement(){
        Expression* right = expression();
        consume(SEMICOLON, "Expected ; after expression.");
        return new ExpressionStatement(right, getLine());
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
                return new Assign(right,left, getLine(), variable->m_name.original);
            }else if(Brackets* variable = dynamic_cast<Brackets*>(left)){
                return new Assign(right,left, getLine(), "brakcets");
            }
            else if (Pointer* ptr = dynamic_cast<Pointer*>(left)){

                return new Assign(right,left, getLine(), ptr->m_variable->m_name.original);
            }
            else if (Get* obj = dynamic_cast<Get*>(left)){

                return new Set(right,obj->m_name, obj->m_object, getLine());
            }
            m_error_handler->error(m_tokens[m_current-1].line, "Assignment error.");
        }
        return left;
    }

    Expression* or_(){
        Expression* left = and_();
        while (match({OR})){
            Token operation_ =   m_tokens[m_current-1];
            Expression* right = and_();
            left= new Logic(left, operation_, right, getLine());
        }
        return left;
    }

    Expression* and_(){
        Expression* left = equality();
        while (match({AND})){
            Token operation_ =   m_tokens[m_current-1];
            Expression* right = equality();
            left= new Logic(left, operation_, right, getLine());
        }
        return left;
    }

    Expression* equality(){
        Expression* left= comparison();
        while (match({BANG_EQUAL, EQUAL_EQUAL})) {
            Token operation_ =  m_tokens[m_current-1];
            Expression* right = comparison();
            left= new Binary(left, operation_, right, getLine());
        }
        return left;
    }
 Expression* comparison() {
     Expression* left= term();
        while (match({GREATER, GREATER_EQUAL, LESS, LESS_EQUAL})) {
            Token operation_ =  m_tokens[m_current-1];
            Expression* right = term();
            left= new Binary(left, operation_, right, getLine());
        }
        return left;
    }
 Expression* term() {Expression* left= factor();

     if(match({INC})){
         Token add; add.original = "+";add.type = PLUS;   add.line = getLine();
         if (Variable* variable = dynamic_cast<Variable*>(left)) {
             //enjoy a nightmare line
             return new Assign(new Binary(left, add, new Literal(1, getLine()), getLine()), variable, getLine(),variable->m_name.original);
         }
         m_error_handler->error(m_tokens[m_current-1].line, "Cannot increment non variable");
     }
     else if(match({DEC})){
         Token subtract;  subtract.original = "-";subtract.type = MINUS;  subtract.line = getLine();
         if (Variable* variable = dynamic_cast<Variable*>(left)) {
             return new Assign(new Binary(left, subtract, new Literal(1, getLine()), getLine()), variable, getLine(),variable->m_name.original);
         }
         m_error_handler->error(m_tokens[m_current-1].line, "Cannot decrement non variable");
     }

     while (match({MINUS, PLUS})) {
            Token operation_ = m_tokens[m_current-1];
            Expression* right = factor();
            left= new Binary(left, operation_, right, getLine());
        }
        return left;
    }
    Expression*  factor() {
        Expression*  left= unary();
        while (match({SLASH, STAR})) {
            Token operation_  = m_tokens[m_current-1];
            Expression* right = unary();
            left= new Binary(left, operation_, right, getLine());
        }
        return left;
    }
    Expression* unary() {
        if (match({BANG, MINUS})) {
            Token operation_ = m_tokens[m_current-1];
            Expression* right = unary();
            return new Unary(operation_, right, getLine());
        }
        return memoryExpression();
    }
    Expression* memoryExpression(){
        if(match({NEW})){
            Expression* right = callExpression();
            return new Memory(right, NEW, getLine());
        }else if(match({DELETE})){
            Expression* right = callExpression();
            return new Memory(right, DELETE, getLine());
        }else if(match({DESTRUCT})){
            Expression* right = callExpression();
            return new Memory(right, DESTRUCT, getLine());
        }
        return callExpression();
    }
    Expression* callExpression(){
        Expression* left = bracketExpression();
        while(true){
            if(match({LEFT_PAREN})){
            left = callFinalize(left);}
            else if(match({DOT})){
                Token name = consume(IDENTIFIER,
                                     "Expected property m_visitor_name");
                left = new Get(left,name, getLine());
            }else{
                break;
            }
        }
        return left;
    }

    Expression* bracketExpression(){
        Expression* left = nullExpression();

        if(match({LEFT_BRACKET})){
            Expression* inside = expression();

            Token paren = consume(RIGHT_BRACKET,
                                  "Expect ']' after contents.");
            left = new Brackets(left, inside, getLine());}
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

        return  new Call(callee, paren, arguments, getLine());
    }


    Expression* nullExpression(){
        if(match({NULLPTR})){
            match({IDENTIFIER, VAR});
            Token type = m_tokens[m_current-1];
            return new TypeExpression(type, NULLPTR, getLine(), nullptr);
        }

        if(match({ARRAY})){
            match({IDENTIFIER, VAR});
            Token type = m_tokens[m_current-1];
            return new TypeExpression(type, ARRAY, getLine(), expression());
        }
        if(match({LOCALARRAY})){
            match({IDENTIFIER, VAR});
            Token type = m_tokens[m_current-1];
            return new TypeExpression(type, LOCALARRAY, getLine(), expression());
        }
        return primary();
    }
    Expression* primary() {
        if (match({FALSE})) return new Literal(false, getLine());
        if (match({TRUE})) return new Literal(true, getLine());
        if (match({NIL})) return new Literal(null_object(), getLine());

        if (match({FLOATING,INTEGER, STRING})) {
            return new Literal(m_tokens[m_current-1].value, getLine());
        }
        if(match({REFERENCE})){
            if(match({IDENTIFIER})){
                return new Pointer(new Variable(m_tokens[m_current-1], getLine(), false), getLine());
            }
        }

        if (match({IDENTIFIER})) {
            return new Variable(m_tokens[m_current-1], getLine(),false);
        }
        if (match({LEFT_PAREN})) {
            Expression* middle = expression();
            consume(RIGHT_PAREN, "Expected ) after (."); //after expression check for right parentheses
            return new Grouping(middle, getLine());
        }
        //does not parse to anything
        m_error_handler->error(m_tokens[m_current].line, "Expected expression.");
        return nullptr;
    }
};

#endif PROGRAM_PARSER_HPP