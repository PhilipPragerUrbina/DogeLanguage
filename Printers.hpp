//
// Created by Philip on 5/22/2022.
//

#ifndef PROGRAM_PRINTERS_HPP
#define PROGRAM_PRINTERS_HPP

#include "Expression.hpp"

//generate a .dot file from AST
class DotFileGenerator : public Visitor{
public:
    //output tree
    void print(statementList statements) {
       m_visitor_name = "Dot File Printer";
        std::cout << "\n digraph G { \n"; //Graph vis syntax
        //for each nod in tree
        for(Statement* statement:statements){
            statement->accept(this);
        }
        std::cout << "} \n";
    }

    object visitFunctionStatement(FunctionStatement *statement) {
        createNode(statement->m_name.original, {},statement->m_body);
        return 0;
    }
    object visitIfStatement(IfStatement* statement){
        createNode("if", {statement->m_condition},{ statement->m_then_branch, statement->m_else_branch});
        return 0;
    }
    object visitBlockStatement(BlockStatement* statement){
        createNode("block", {}, statement->m_statements);
        return 0;
    }
    object visitWhileStatement(WhileStatement* statement){

        createNode("while", {statement->m_condition}, {statement->m_body});


        return 0;
    }
    object visitVariableStatement(VariableStatement* statement){
        if(statement->m_initializer != nullptr){
            createNode(statement->m_name.original, {statement->m_initializer});
        }else{
            createNode(statement->m_name.original, {});
        }
        return 0;
    }
    object visitAssignExpression(Assign* expression){
        createNode(expression->m_name.original, {expression->m_value});
        return 0;
    }
    object visitVariableExpression(Variable* expression){
        createNode(expression->m_name.original, {});
        return 0;
    }

    object visitExpressionStatement(ExpressionStatement* statement){
        createNode("statement ", {statement->m_expression});
        return 0;
    }
    object visitReturnStatement(ReturnStatement *statement){
        if(statement->m_value != nullptr){
            createNode("return ", {statement->m_value});
        }else{
            createNode("return ", {});
        }
        return 0;
    }
    object visitLogicExpression(Logic *expression){

        if(expression->m_operator_.type == OR){
            createNode("OR ", {expression->m_left, expression->m_right});
        }else{
            createNode("AND ", {expression->m_left, expression->m_right});
        }
        return 0;
    }
    object visitCallExpression(Call* expression) {
        std::vector<Expression*> a = expression->m_arguments;
        a.push_back(expression->m_callee);
        createNode("call ", a );


        return 0;
    }


    object visitPointerExpression(Pointer *expression){
        createNode(expression->m_variable->m_name.original + " pointer", {});
        return 0;
    }
    object visitBinaryExpression(Binary* expression){
        createNode(expression->m_operator_.original,{expression->m_left, expression->m_right});
        return 0;
    };
    object visitGroupingExpression(Grouping* expression){
        createNode("Group",{expression->m_expression});
        return 0;
    };
    object visitLiteralExpression(Literal* expression){
        std::string value = "object";
        if(std::get_if<std::string>(&expression->m_value) ){
            value = std::get<std::string>(expression->m_value);
        }else if(std::get_if<float>(&expression->m_value)){
            value = std::to_string(std::get< float>(expression->m_value));
        }else if(std::get_if<int>(&expression->m_value)){
            value = std::to_string(std::get< int>(expression->m_value));
        }
        createNode(value,{});
        return 0;
    };
    object visitUnaryExpression(Unary* expression){
        createNode(expression->m_operator_.original,{expression->m_right});
        return 0;
    };
private:
    //for creating a unique identifier for each node
    int m_id = 0;
    //keep track of parent nodes
    std::vector<std::string> m_prev;

    void createNode(std::string name, std::vector<Expression*> children,std::vector<Statement*> s_children = {}){
        m_id ++; //increment id
        name = std::to_string(m_id) + ": " + name; //make sure m_visitor_name is unique
        if(!m_prev.empty()){std::cout << "\"" + m_prev[m_prev.size()-1] + "\" -> \"" + name + "\"; \n";}else{
            std::cout << "\" program \" -> \"" + name + "\"; \n";
        } //output node if parent has been created
        m_prev.push_back(name); //add to list
        for(Expression* expr : children){expr->accept(this);} //gen children
        for(Statement* expr : s_children){
            if(expr != nullptr){   expr->accept(this);}
         } //gen children
        m_prev.pop_back(); //pop from list
    }


};


#endif PROGRAM_PRINTERS_HPP