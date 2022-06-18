//
// Created by Philip on 6/9/2022.
//

#ifndef PROGRAM_ANALYZER_HPP
#define PROGRAM_ANALYZER_HPP
#include "Environment.hpp"

class Analyzer : public Visitor{
public:
    //keep track of file names for errors
    std::vector<std::string> m_names;
    std::map<std::string,statementList> m_external_files;
    //run the Analyzer. return true if ok.
    bool check(statementList statements, std::map<std::string,statementList> external_files,ErrorHandler* error_handler, std::string file) {
        //setup error handler
        m_error_handler = error_handler;
        m_error_handler->m_name = "Analyzer";
        m_external_files = external_files;
        m_error_handler->m_file = file;
        //set up main env
        m_environment = new Environment();
        m_top = m_environment;
        m_names.push_back(file);
        //run each statement
        for(Statement* statement:statements){
            statement->accept(this);
        }
        m_names.pop_back();
        return m_error_handler->hasErrors();
    }

    //statements
    object visitImportStatement(ImportStatement *statement){
        if (m_external_files.find(statement->m_name.original) == m_external_files.end() ) {
            m_error_handler->error(statement->m_line, "Cannot find import: " + statement->m_name.original);
            return std::string("null");
        }
        statementList statements = m_external_files[statement->m_name.original];
        m_names.push_back(statement->m_name.original);
        m_error_handler->m_file = m_names.back();
        for (Statement *new_statement: statements) {
            new_statement->accept(this);
        }
        m_names.pop_back();
        m_error_handler->m_file = m_names.back();
        return std::string("null");
    }

    object visitIncludeStatement(IncludeStatement *statement) {
        if(statement->m_link){
            //check if c++ file or precompiled file which are supported
            std::filesystem::path file_path = std::get<std::string>(statement->m_name.value);
            if(file_path.extension() != ".o" && file_path.extension() != ".cpp") {
                m_error_handler->error(statement->m_line, "Cannot link this file: " + std::get<std::string>(statement->m_name.value));
            }
            return std::string("null");
        }
        if (m_external_files.find(std::get<std::string>(statement->m_name.value)) == m_external_files.end() ) {
            m_error_handler->error(statement->m_line, "Cannot find include: " + std::get<std::string>(statement->m_name.value));
            return std::string("null");
        }
        statementList statements = m_external_files[std::get<std::string>(statement->m_name.value)];
        m_names.push_back(statement->m_name.original);
        m_error_handler->m_file = m_names.back();
        for (Statement *new_statement: statements) {
            new_statement->accept(this);
        }
        m_names.pop_back();
        m_error_handler->m_file = m_names.back();
        return std::string("null");
    }

    object visitClassStatement(ClassStatement *statement) {
        Class new_class(statement->m_name.original, new Environment(m_environment));
        m_environment->define(statement->m_name.original,new_class);
        executeBlock(statement->m_members,new_class.m_environment, false);
        return std::string("null");
    }
    object visitFunctionStatement(FunctionStatement *statement) {
        Callable function =  Callable(statement->m_parameters.size(), nullptr,statement);
        std::string overload = "";
        for (int i = 0; i < statement->m_parameters.size(); i++) {
                overload = overload + "_" + statement->m_parameters[i]->m_type.original;

        }
        if(statement->m_class_name != ""){
            overload = overload + "_" + statement->m_class_name+"_";
        }

        m_top->define(statement->m_name.original + "_function" + overload ,function);
        m_top->define(statement->m_name.original + "_function" ,function);
        std::string name = statement->m_name.original ;
        if(name == statement->m_class_name){
            name = name + "_constructor";
        }

        m_environment->define(name ,statement->m_name.original + "_function" );
        if(statement->m_body.empty()){
            return (std::string)"null";
        }
        //set up scope
        Environment* environment;
        if(statement->m_class_name != ""){
            environment = new Environment(std::get<Class>(m_environment->getValue(statement->m_class_name)).m_environment);
            //add this variable
            environment->define("this",statement->m_class_name);
        }else{
            environment = new Environment(m_top);
        }
        for (int i = 0; i < statement->m_parameters.size(); i++) {
            environment->define(statement->m_parameters[i]->m_name.original,
                                statement->m_parameters[i]->m_type.original);
        }
        m_function = true;
        executeBlock(statement->m_body, environment);
        m_function = false;
        if(m_return == "" && statement->m_type.original != "void"){
            m_error_handler->error(statement->m_line,"No return value.");
        }
        m_return  = "";
        return (std::string)"null";
    }
    object visitIfStatement(IfStatement* statement){
        evalS(statement->m_condition);
         statement->m_then_branch->accept(this);
        if(statement->m_else_branch != nullptr ){
           statement->m_else_branch->accept(this);
        }
        return (std::string)"null";
    }
    object visitWhileStatement(WhileStatement* statement){
        evalS(statement->m_condition);
        m_loop_num++;
        statement->m_body->accept(this);
        m_loop_num--;
        return (std::string)"null";
    }
    bool m_function = false;
    std::string m_return = "";
    int m_loop_num = 0;
    object visitReturnStatement(ReturnStatement *statement){

        if(statement->m_keyword.original == "break" || statement->m_keyword.original == "continue"){
            if(m_loop_num <= 0){
                m_error_handler->error(statement->m_line, statement->m_keyword.original  + " is not in loop.");
            }
            return (std::string)"null";
        }
        if(!m_function){
            m_error_handler->error(statement->m_line, "Return is not in function.");
        }
        m_return = "null";
        if(statement->m_value!= nullptr){
            m_return = evalS(statement->m_value);
        }
        return (std::string)"null";
    }

    object visitBlockStatement(BlockStatement* statement){
        return executeBlock(statement->m_statements, new Environment(m_environment));
    }

    object visitVariableStatement(VariableStatement* statement){
        std::string type = statement->m_type.original;
        if(statement->m_initializer != nullptr){
            std::string value = evalS(statement->m_initializer);
            if(value != type){
                if(type != "float" && value != "int" ) {
                    m_error_handler->error(statement->m_line, "Cannot assign " + value + " to " + type);
                }
            }
        }
        if(m_environment->define(statement->m_name.original,type,statement->m_constant)){m_error_handler->error(statement->m_line, "Variable Already defined: " + statement->m_name.original);};
        return (std::string)"null";
    }

    object visitExpressionStatement(ExpressionStatement* statement){
        return evalS(statement->m_expression);

    }


    //expressions
    object visitCallExpression(Call* expression) {
        std::string callee = evalS(expression->m_callee);
        object callee_obj = m_environment->getValue(callee);



        std::string overload = "";
        for (int i = 0; i < expression->m_arguments.size(); i++) {
            overload = overload + "_" + evalS(expression->m_arguments[i]);

        }

        if(Callable* function = std::get_if<Callable>(&callee_obj)) {
                if(function->m_class != ""){
                        overload = overload + "_" + function->m_declaration->m_class_name+"_";}
        }

        //if class check constructor
        std::string class_return = "";
        if(Class* class_type = std::get_if<Class>(&callee_obj)) {
            class_return =  class_type->m_name;
            callee_obj = m_environment->getValue(class_type->m_name + "_function" + overload + "_" + class_type->m_name+"_");
            if(expression->m_arguments.empty()){
                //ignore default constructor
                if(std::get_if<null_object>(&callee_obj)){
                    return  class_return;
                }
            }
        }
        if(class_return == ""){
            callee_obj = m_environment->getValue(callee + overload);
        }



        if(Callable* function = std::get_if<Callable>(&callee_obj)){

            int argument_number = 0;

            for (Expression* argument : expression->m_arguments) {
                if (argument_number+1 > function->m_argument_number) {
                    m_error_handler->error("too many function arguments: " + std::to_string(argument_number) + " expected: " +
                                           std::to_string(function->m_argument_number) + function->m_declaration->m_name.original + overload); return (std::string)"null";}

                std::string type  = evalS(argument);

                if(type != function->m_declaration->m_parameters[argument_number]->m_type.original){
                    m_error_handler->error(expression->m_line,"Argument " + std::to_string(argument_number) + " is " +
                    function->m_declaration->m_parameters[argument_number]->m_type.original + " not " + type);
                }
                argument_number++;
            }
            if (argument_number != function->m_argument_number) {
                m_error_handler->error("Not enough function arguments: " + std::to_string(argument_number) + " expected: " +
                                       std::to_string(function->m_argument_number));}
        if(class_return != ""){
            return class_return; //return constructor type
        }
            return function->m_declaration->m_type.original;
        }



        m_error_handler->error(expression->m_line, "Can not call non callable: " + callee + overload);
        return (std::string)"null";
    }

    object visitLogicExpression(Logic *expression){
        evalS(expression->m_left);
        evalS(expression->m_right);
        return (std::string)"bool";
    }

    object visitAssignExpression(Assign* expression){
        std::string value = evalS(expression->m_value);
        object get = m_environment->getValue(expression->m_name);
        if(std::get_if<std::string>(&get)){
        if(value != std::get<std::string>(get)){
            if(std::get<std::string>(get) != "float" && value != "int" ) {
                m_error_handler->error(expression->m_line, "Cannot assign different type.");
            }
            return (std::string)"null";
        }

        if(m_environment->isConst(expression->m_name)){
            m_error_handler->error(expression->m_line,"Attempt to write constant.");
            return (std::string)"null";
        }
        }else{
            m_error_handler->error(expression->m_line,"Assignment undefined: " + expression->m_name);
        }

        return (std::string)"null";
    }

    object visitVariableExpression(Variable* expression) {
        object value = m_environment->getValue(expression->m_name.original);
        if (std::get_if<null_object>(&value)) {
            m_error_handler->error(expression->m_line, "Reference undefined: " + expression->m_name.original);
            return (std::string) "null";
        }
        if (Class *class_type = std::get_if<Class>(&value)) {
            return class_type->m_name;
        }
        return value;
    }
    object visitPointerExpression(Pointer *expression) {
        std::string variable = evalS(expression->m_variable);
        //dereference
        if(variable.find("_ptr") != std::string::npos){
            variable.erase(variable.length()-4);
            return variable;
        }else{
            //reference
            return variable + "_ptr";
        }
    }


    object visitGetExpression(Get *expression) {
        std::string value = evalS(expression->m_object);
        object class_ = m_environment->getValue(value);
        if (Class* obj = std::get_if<Class>(&class_)) {
            object out =  obj->m_environment->getValue(expression->m_name.original);
            if(!std::get_if<std::string>(&out)){
                m_error_handler->error(expression->m_line,"Unknown member.");
            }

            return out;
        }
        m_error_handler->error(expression->m_line,"Cannot read property of non object.");
        return (std::string)"null";
    }

    object visitSetExpression(Set *expression) {
        std::string  left = evalS(expression->m_object);
        object class_ = m_environment->getValue(left);
        if(Class* obj = std::get_if<Class>(&class_)){
            std::string right = evalS(expression->m_value);
            std::string type = std::get<std::string>(obj->m_environment->getValue(expression->m_name.original));
            if(!obj->m_environment->assign(expression->m_name.original, type)){
                m_error_handler->error(expression->m_line,"Cannot write unknown member.");
            };
            if(type != right){
                if(type != "float" && right != "int" ){
                    m_error_handler->error(expression->m_line,"Cannot set different type.");
                }
            }
            return (std::string)"null";
        }
        m_error_handler->error(expression->m_line,"Cannot write property of non object.");
        return (std::string)"null";
    }


    object visitBinaryExpression(Binary* expression){
        std::string right = evalS(expression->m_right);
        std::string left = evalS(expression->m_left);
        switch (expression->m_operator_.type) {
            case BANG_EQUAL:
                if(left == "bool" && right == "bool"){
                    return std::string("bool");
                }
                if(left == "float" && right == "float"){
                    return (std::string)"float";
                }
                if(left == "int" && right == "int"){
                    return std::string("int");
                } else{
                    return checkOperator(left,right, "bangEqual");
                }

            case EQUAL_EQUAL:
                if(left == "bool" && right == "bool"){
                    return (std::string)"bool";
                }
                if(left == "float" && right == "float"){
                    m_error_handler->warning(expression->m_line,"Comparing two floats directly is unreliable.");
                    return (std::string)"float";
                }
                if(left == "int" && right == "int"){
                    return (std::string)"int";
                } else{
                    return checkOperator(left,right, "equalEqual");
                }
            case GREATER:
                if(left == "float" && right == "float"){
                    return (std::string)"float";
                }
                if(left == "int" && right == "int"){
                    return (std::string)"int";
                } else{
                    return checkOperator(left,right, "greater");
                }
            case GREATER_EQUAL:
                if(left == "float" && right == "float"){
                    return (std::string)"float";
                }
                if(left == "int" && right == "int"){
                    return (std::string)"int";
                } else{
                    return checkOperator(left,right, "greaterEqual");
                }
            case LESS:
                if(left == "float" && right == "float"){
                    return (std::string)"float";
                }
                if(left == "int" && right == "int"){
                    return (std::string)"int";
                } else{
                    return checkOperator(left,right, "less");
                }
            case LESS_EQUAL:
                if(left == "float" && right == "float"){
                    return (std::string)"float";
                }
                if(left == "int" && right == "int"){
                    return std::string("int");
                } else{
                    return checkOperator(left,right, "lessEqual");
                }
            case MINUS:
                if(left == "float" && right == "float"){
                    return std::string("float");
                }
                if(left == "int" && right == "int"){
                    return std::string("int");
                } else{
                    return checkOperator(left,right, "minus");
                }
            case SLASH:
                if(left == "float" && right == "float"){
                    return std::string("float");
                }
                if(left == "int" && right == "int"){
                    return std::string("int");
                } else{
                    return checkOperator(left,right, "slash");
                }
            case STAR:
                if(left == "float" && right == "float"){
                    return std::string("float");
                }
                if(left == "int" && right == "int"){
                    return std::string("int");
                } else{
                    return checkOperator(left,right, "star");
                }
            case PLUS:
                if(left == "float" && right == "float"){
                    return std::string("float");
                }
                 if(left == "int" && right == "int"){
                    return std::string("int");
                }
                else{
                    return checkOperator(left,right, "plus");
                }
        }
        m_error_handler->error(expression->m_line,"Not a binary type for: " + expression->m_operator_.original);
        return std::string("null");
    };
    object visitGroupingExpression(Grouping* expression){
        return evalS(expression->m_expression);
    };
    object visitLiteralExpression(Literal* expression){
        if(int* number =std::get_if<int>(& expression->m_value)){ return std::string ("int");}
        if(float* number =std::get_if<float>(& expression->m_value)){ return (std::string)"float";}
        if(bool* boolean =std::get_if<bool>(& expression->m_value)){ return (std::string)"bool";}
        if(std::string* s =std::get_if<std::string>(& expression->m_value)){return (std::string)"chars";}
        m_error_handler->error(expression->m_line,"Unknown type.");
        return std::string("null");
    };
    object visitUnaryExpression(Unary* expression){
        std::string right = evalS(expression->m_right);
        switch (expression->m_operator_.type) {
            case BANG:
                if(right == "bool"){
                    return std::string("bool");
                }
            case MINUS:
                if(right == "int"){
                    return std::string("int");
                }
                if(right == "float"){
                    return std::string("float");
                } else{
                    return checkUnaryOperator(right, "negative");
                }
        }
        m_error_handler->error(expression->m_line,"Not a unary type.");
        return std::string("null");
    };
private:
    Environment* m_environment;
    Environment* m_top;
    ErrorHandler* m_error_handler;
    std::string evalS(Expression* expression){
        return std::get<std::string> (expression->accept(this));
    }


    object executeBlock(statementList statements, Environment* environment, bool cleanup = true){
        Environment* pre = m_environment;
        m_environment = environment;
        for(Statement* inner_statement: statements){
            inner_statement->accept(this);
        }
        if( cleanup){
            delete environment;
        }
        m_environment = pre;
        return std::string("null");
    }
    //overloading checkers
    std::string checkOperator(std::string left, std::string right, std::string op){
        //check conversions
        if(left == "float" && right == "int"){
            return "float";
        }
        if(right == "float" && left == "int"){
            m_error_handler->error("Casting a float to a int is narrowing!");
        }
        std::string callee = op + "_function_" + right + "_" + left+"_";
        object callee_obj = m_environment->getValue(callee);
        if(Callable* function = std::get_if<Callable>(&callee_obj)){
            if(function->m_argument_number != 1){
                m_error_handler->error( "Overloaded binary operator must have 1 additional argument: " + callee);
            }
            return function->m_declaration->m_type.original;
        }
        m_error_handler->error("Overloaded operator does not exist: " + callee);
        return (std::string)"null";
    }
    std::string checkUnaryOperator(std::string right, std::string op){
        std::string callee = op + "_function_" + right+"_";
        object callee_obj = m_environment->getValue(callee);
        if(Callable* function = std::get_if<Callable>(&callee_obj)){
            if(function->m_argument_number != 0){
                m_error_handler->error( "Overloaded unary operator must mo arguments: " + callee);
            }
            return function->m_declaration->m_type.original;
        }
        m_error_handler->error("Overloaded unary operator does not exist: " + callee);
        return (std::string)"null";
    }
};



#endif //PROGRAM_ANALYZER_HPP
