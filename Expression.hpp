//
// Created by Philip on 5/21/2022.
//

#ifndef PROGRAM_EXPRESSION_HPP
#define PROGRAM_EXPRESSION_HPP

#include "Scanner.hpp"


//pre declare variables
class Binary;
class Grouping;
class Literal;
class Unary;
class ExpressionStatement;
class IfStatement;
class Variable;
class VariableStatement;
class Assign;
class BlockStatement;
class Logic;
class WhileStatement;
class Call;
class FunctionStatement;
class ReturnStatement;
class Pointer;
class ClassStatement;
class Get;
class Set;
class ImportStatement;
class IncludeStatement;
class Memory;
class Brackets;
class EmptyStatement;

//using visitor pattern
class Visitor {
public:
    std::string m_visitor_name = "Unknown";
    virtual  object visitBinaryExpression(Binary* expression){std::cout << m_visitor_name << " visitBinaryExpression" << " not implemented \n"; return 0;};
    virtual  object visitGroupingExpression(Grouping* expression){std::cout << m_visitor_name << " visitGroupingExpression" << " not implemented \n"; return 0;};
    virtual  object visitLiteralExpression(Literal* expression){std::cout << m_visitor_name << " visitLiteralExpression" << " not implemented \n";return 0;};
    virtual  object visitUnaryExpression(Unary* expression){std::cout << m_visitor_name << " visitUnaryExpression" << " not implemented \n";return 0;};
    virtual object visitExpressionStatement(ExpressionStatement* statement){std::cout << m_visitor_name << " visitExpressionStatement " << " not implemented \n";return 0;};
    virtual  object visitVariableExpression(Variable* expression){std::cout << m_visitor_name << " visitVariableExpression " << " not implemented \n";return 0;};
    virtual  object visitLogicExpression(Logic* expression){std::cout << m_visitor_name << " visitLogicExpression " << " not implemented \n";return 0;};
    virtual  object visitVariableStatement(VariableStatement* statement){std::cout << m_visitor_name << " visitVariableStatement " << " not implemented \n";return 0;};
    virtual  object visitBlockStatement(BlockStatement* statement){std::cout << m_visitor_name << " visitBlockStatement " << " not implemented \n";return 0;};
    virtual  object visitAssignExpression(Assign* expression){std::cout << m_visitor_name << " visitAssignExpression " << " not implemented \n";return 0;};
    virtual  object visitIfStatement(IfStatement* statement){std::cout << m_visitor_name << " visitIfStatement " << " not implemented \n";return 0;};
    virtual  object visitFunctionStatement(FunctionStatement* statement){std::cout << m_visitor_name << " visitFunctionStatement " << " not implemented \n";return 0;};
    virtual object visitWhileStatement(WhileStatement* statement){std::cout << m_visitor_name << " visitWhileStatement " << " not implemented \n";return 0;};
    virtual object visitCallExpression(Call* expression){std::cout << m_visitor_name << " visitCallExpression " << " not implemented \n";return 0;};
    virtual object visitReturnStatement(ReturnStatement* statement){std::cout << m_visitor_name << " visitReturnStatement " << " not implemented \n";return 0;};
    virtual object visitClassStatement(ClassStatement* statement){std::cout << m_visitor_name << " visitClassStatement " << " not implemented \n";return 0;};
    virtual object visitImportStatement(ImportStatement* statement){std::cout << m_visitor_name << " visitImportStatement " << " not implemented \n";return 0;};
    virtual object visitIncludeStatement(IncludeStatement* statement){std::cout << m_visitor_name << " visitIncludeStatement " << " not implemented \n";return 0;};
    virtual object visitPointerExpression(Pointer* expression){std::cout << m_visitor_name << " visitPointerExpression " << " not implemented \n";return 0;}
    virtual object visitGetExpression(Get* expression){std::cout << m_visitor_name << " visitGetExpression " << " not implemented \n";return 0;}
    virtual object visitSetExpression(Set* expression){std::cout << m_visitor_name << " visitSetExpression " << " not implemented \n";return 0;}
    virtual object visitBracketsExpression(Brackets* expression){std::cout << m_visitor_name << " visitBracketsExpression " << " not implemented \n";return 0;}
    virtual object visitMemoryExpression(Memory* expression){std::cout << m_visitor_name << " visitMemoryExpression " << " not implemented \n";return 0;}
    virtual object visitEmptyStatement(EmptyStatement* statement){return 0;};

};

//base class
class Expression {
public:
    virtual object accept(Visitor* visitor){return 0;};
    int m_line = 0;
};

//derived classes
class Binary : public Expression {
public:
    Binary(Expression* left, Token operator_, Expression* right, int line) {
        m_left = left;
        m_operator_ = operator_;
        m_right = right;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitBinaryExpression(this);
    }

    Expression* m_left;
    Token m_operator_;
    Expression* m_right;
};
class Brackets : public Expression {
public:
    Brackets(Expression* left, Expression* right, int line) {
        m_left = left;
        m_right = right;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitBracketsExpression(this);
    }

    Expression* m_left;
    Expression* m_right;
};

class Call : public Expression {
public:
    Call(Expression* callee, Token paren, std::vector<Expression*> arguments, int line) {
        m_callee = callee;
        m_paren = paren;
        m_arguments = arguments;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitCallExpression(this);
    }

    Expression* m_callee;
    Token m_paren;
    std::vector<Expression*> m_arguments;
};

class Memory : public Expression {
public:
    Memory(Expression* right,  TokenType type, int line) {
        m_right = right;
        m_line = line;
        m_type = type;
    }

    object accept(Visitor* visitor) {
        return visitor->visitMemoryExpression(this);
    }
    TokenType m_type;
    Expression* m_right;
};

class Get : public Expression {
public:
    Get(Expression* class_object, Token name, int line) {
        m_object = class_object;
        m_name = name;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitGetExpression(this);
    }

    Expression* m_object;
    Token m_name;
};
class Logic : public Expression {
public:
    Logic(Expression* left, Token operator_, Expression* right, int line) {
        m_left = left;
        m_operator_ = operator_;
        m_right = right;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitLogicExpression(this);
    }

    Expression* m_left;
    Token m_operator_;
    Expression* m_right;
};
class Grouping : public Expression {
public:
    Grouping(Expression* expression, int line) {
        m_expression = expression;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitGroupingExpression(this);
    }

    Expression* m_expression;
};
class Literal : public Expression {
public:
    Literal(object value, int line) {
        m_value = value;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitLiteralExpression(this);
    }

    object m_value;
};
class Unary : public Expression {
public:
    Unary(Token operator_, Expression* right, int line) {
        m_operator_ = operator_;
        m_right = right;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitUnaryExpression(this);
    }

    Token m_operator_;
    Expression* m_right;
};
class Variable : public Expression {
public:
    Variable(Token name, int line, bool constant) {
        m_name = name;
        m_constant = constant;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitVariableExpression(this);
    }
    bool m_constant;
    Token m_name;
};
class Pointer : public Expression {
public:
    Pointer(Variable* variable, int line) {
        m_variable = variable;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitPointerExpression(this);
    }

    Variable* m_variable;
};


class Set : public Expression {
public:
    Set(Expression* value, Token name, Expression* obj, int line) {
        m_name = name;
        m_value = value;
        m_object = obj;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitSetExpression(this);
    }
    Expression* m_object;
    Token m_name;
    Expression* m_value;

};
//statements

class Statement {
public:

    virtual object accept(Visitor* visitor){return  0;};
    int m_line = 0;
};
typedef std::vector<Statement*> statementList;
class ReturnStatement : public Statement {
public:
    ReturnStatement(Token keyword, Expression* value, int line) {
        m_value = value;
        m_keyword = keyword;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitReturnStatement(this);
    }

    Expression* m_value;
    Token m_keyword;
};

class EmptyStatement : public Statement {
public:
    EmptyStatement() {

    }

    object accept(Visitor* visitor) {
        return visitor->visitEmptyStatement(this);
    }


};
class FunctionStatement : public Statement {
public:
    FunctionStatement(Token name, std::vector<VariableStatement*> parameters, statementList body,Token type, int line, std::string class_name) {
        m_name = name;
        m_parameters = parameters;
        m_body = body;
        m_line = line;
        m_type = type;
       m_class_name = class_name;
    }

    object accept(Visitor* visitor) {
        return visitor->visitFunctionStatement(this);
    }
    std::string m_class_name;
    Token m_type;
    statementList m_body;
    Token m_name;
    std::vector<VariableStatement*> m_parameters;
};
class ClassStatement : public Statement {
public:
    ClassStatement(Token name, std::vector<Statement*> members, int line, std::string template_name = "", std::string template_type = "") {
        m_name = name;
        m_members = members;
        m_line = line;
        m_template_name =template_name;
        m_template_type =template_type;
    }

    object accept(Visitor* visitor) {
        return visitor->visitClassStatement(this);
    }
    Token m_name;
    std::string m_template_name;
    std::string m_template_type;
    std::vector<Statement*> m_members;
};

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(Expression* expression, int line) {
        m_expression = expression;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitExpressionStatement(this);
    }
    Expression* m_expression;
};

class ImportStatement : public Statement {
public:
    ImportStatement(Token name,int line) {
        m_name = name;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitImportStatement(this);
    }
    Token m_name;
};
class IncludeStatement : public Statement {
public:
    IncludeStatement(Token name, int line, bool link) {
        m_name = name;
        m_line = line;
        m_link = link;
    }

    object accept(Visitor* visitor) {
        return visitor->visitIncludeStatement(this);
    }
    bool m_link;
    Token m_name;
};

class VariableStatement : public Statement {
public:
    VariableStatement(Expression* initializer, Token name, bool constant, Token type, int line) {
        m_initializer = initializer;
        m_name = name;
        m_constant = constant;
        m_line = line;
        m_type = type;
    }

    object accept(Visitor* visitor) {
        return visitor->visitVariableStatement(this);
    }
    Token m_type;
    bool m_constant;
    Expression* m_initializer;
    Token m_name;
};
class BlockStatement : public Statement {
public:
    BlockStatement(statementList statements, int line) {
        m_statements = statements;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitBlockStatement(this);
    }

    statementList m_statements;
};
class IfStatement : public Statement {
public:
    IfStatement(Expression* condition, Statement* then_branch, Statement* else_branch, int line) {
        m_condition = condition;
        m_then_branch = then_branch;
        m_else_branch = else_branch;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitIfStatement(this);
    }

    Expression* m_condition;
    Statement* m_then_branch;
    Statement* m_else_branch;
};
class WhileStatement : public Statement {
public:
    WhileStatement(Expression* condition, Statement* body, int line) {
        m_condition = condition;
        m_body = body;
        m_line = line;
    }

    object accept(Visitor* visitor) {
        return visitor->visitWhileStatement(this);
    }

    Expression* m_condition;
    Statement* m_body;
};
//THis expression is down here because it requires as statement for a special constructor
class Assign : public Expression {
public:
    Assign(Expression* value, Expression* variable,int line, std::string name) {
        m_variable = variable;
        m_value = value;
m_name = name;
        m_line = line;

    }
    //create assign from variable statement for members
    Assign(VariableStatement* statement, bool destroy = false){
        m_variable = new Variable(Token(statement->m_name),0,false);
        m_name = statement->m_name.original;
        m_value = statement->m_initializer;
        m_line = statement->m_line;
        m_destroy = destroy;

    }

    object accept(Visitor* visitor) {
        return visitor->visitAssignExpression(this);
    }
bool m_destroy = false;
std::string m_name;
   Expression* m_variable;
    Expression* m_value;

};

#endif PROGRAM_EXPRESSION_HPP