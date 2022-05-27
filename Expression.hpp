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
//using visitor pattern
class Visitor {
public:
    virtual  object visitBinaryExpression(Binary* expression){return 0;};
    virtual  object visitGroupingExpression(Grouping* expression){return 0;};
    virtual  object visitLiteralExpression(Literal* expression){return 0;};
    virtual  object visitUnaryExpression(Unary* expression){return 0;};
    virtual object visitExpressionStatement(ExpressionStatement* statement){return 0;};
    virtual  object visitVariableExpression(Variable* expression){return 0;};
    virtual  object visitLogicExpression(Logic* expression){return 0;};
    virtual  object visitVariableStatement(VariableStatement* statement){return 0;};
    virtual  object visitBlockStatement(BlockStatement* statement){return 0;};
    virtual  object visitAssignExpression(Assign* expression){return 0;};
    virtual  object visitIfStatement(IfStatement* statement){return 0;};
    virtual  object visitFunctionStatement(FunctionStatement* statement){return 0;};
    virtual object visitWhileStatement(WhileStatement* statement){return 0;};
    virtual object visitCallExpression(Call* expression){return 0;};
    virtual object visitReturnStatement(ReturnStatement* statement){return 0;};
    virtual object visitPointerExpression(Pointer* expression){return 0;}
};

//base class
class Expression {
public:
    virtual object accept(Visitor* visitor){return 0;};
};

//derived classes
class Binary : public Expression {
public:
    Binary(Expression* left, Token operator_, Expression* right) {
        m_left = left;
        m_operator_ = operator_;
        m_right = right;
    }

    object accept(Visitor* visitor) {
        return visitor->visitBinaryExpression(this);
    }

    Expression* m_left;
    Token m_operator_;
    Expression* m_right;
};

class Call : public Expression {
public:
    Call(Expression* callee, Token paren, std::vector<Expression*> arguments) {
        m_callee = callee;
        m_paren = paren;
        m_arguments = arguments;
    }

    object accept(Visitor* visitor) {
        return visitor->visitCallExpression(this);
    }

    Expression* m_callee;
    Token m_paren;
    std::vector<Expression*> m_arguments;
};
class Logic : public Expression {
public:
    Logic(Expression* left, Token operator_, Expression* right) {
        m_left = left;
        m_operator_ = operator_;
        m_right = right;
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
    Grouping(Expression* expression) {
        m_expression = expression;
    }

    object accept(Visitor* visitor) {
        return visitor->visitGroupingExpression(this);
    }

    Expression* m_expression;
};
class Literal : public Expression {
public:
    Literal(object value) {
        m_value = value;
    }

    object accept(Visitor* visitor) {
        return visitor->visitLiteralExpression(this);
    }

    object m_value;
};
class Unary : public Expression {
public:
    Unary(Token operator_, Expression* right) {
        m_operator_ = operator_;
        m_right = right;
    }

    object accept(Visitor* visitor) {
        return visitor->visitUnaryExpression(this);
    }

    Token m_operator_;
    Expression* m_right;
};
class Variable : public Expression {
public:
    Variable(Token name) {
        m_name = name;
    }

    object accept(Visitor* visitor) {
        return visitor->visitVariableExpression(this);
    }

    Token m_name;
};
class Pointer : public Expression {
public:
    Pointer(Variable* variable) {
        m_variable = variable;
    }

    object accept(Visitor* visitor) {
        return visitor->visitPointerExpression(this);
    }

    Variable* m_variable;
};

class Assign : public Expression {
public:
    Assign(Expression* value, Token name, bool pointer = false) {
        m_name = name;
        m_value = value;
        m_pointer = pointer;
    }

    object accept(Visitor* visitor) {
        return visitor->visitAssignExpression(this);
    }
    bool m_pointer;
    Token m_name;
    Expression* m_value;

};

//statements

class Statement {
public:

    virtual object accept(Visitor* visitor){return  0;};
};
typedef std::vector<Statement*> statementList;
class ReturnStatement : public Statement {
public:
    ReturnStatement(Token keyword, Expression* value) {
        m_value = value;
        m_keyword = keyword;
    }

    object accept(Visitor* visitor) {
        return visitor->visitReturnStatement(this);
    }

    Expression* m_value;
    Token m_keyword;
};
class FunctionStatement : public Statement {
public:
    FunctionStatement(Token name, std::vector<Token> parameters, statementList body) {
        m_name = name;
        m_parameters = parameters;
        m_body = body;
    }

    object accept(Visitor* visitor) {
        return visitor->visitFunctionStatement(this);
    }
    statementList m_body;
    Token m_name;
    std::vector<Token> m_parameters;
};


class ExpressionStatement : public Statement {
public:
    ExpressionStatement(Expression* expression) {
        m_expression = expression;
    }

    object accept(Visitor* visitor) {
        return visitor->visitExpressionStatement(this);
    }
    Expression* m_expression;
};


class VariableStatement : public Statement {
public:
    VariableStatement(Expression* initializer, Token name) {
        m_initializer = initializer;
        m_name = name;
    }

    object accept(Visitor* visitor) {
        return visitor->visitVariableStatement(this);
    }

    Expression* m_initializer;
    Token m_name;
};
class BlockStatement : public Statement {
public:
    BlockStatement(statementList statements) {
        m_statements = statements;
    }

    object accept(Visitor* visitor) {
        return visitor->visitBlockStatement(this);
    }

    statementList m_statements;
};
class IfStatement : public Statement {
public:
    IfStatement(Expression* condition, Statement* then_branch, Statement* else_branch) {
        m_condition = condition;
        m_then_branch = then_branch;
        m_else_branch = else_branch;
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
    WhileStatement(Expression* condition, Statement* body) {
        m_condition = condition;
        m_body = body;
    }

    object accept(Visitor* visitor) {
        return visitor->visitWhileStatement(this);
    }

    Expression* m_condition;
    Statement* m_body;
};


#endif PROGRAM_EXPRESSION_HPP