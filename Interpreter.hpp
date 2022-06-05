//
// Created by Philip on 5/22/2022.
//

#ifndef PROGRAM_INTERPRETER_HPP
#define PROGRAM_INTERPRETER_HPP

#include "Environment.hpp"
#include <chrono>
#include <thread>



class Interpreter : public Visitor{
public:
    ~Interpreter(){
        //clean up threads
        if(m_threaded){
            delete m_error_handler;
        }
    }

    //run the interpreter
    std::string run(statementList statements,ErrorHandler* error_handler) {
        //setup error handler
        m_error_handler = error_handler;
        m_error_handler->m_name = "Runtime";

        m_environment = new Environment();
        m_top = m_environment;
        for(Statement* statement:statements){
            statement->accept(this);
        }
        object out = m_error_handler->getErrorNumber();
        return getString(out);
    }
    //multithreading stuff:
    bool m_threaded = false;
    //run a function
    void runForked(Callable call, Environment* top_env) {
        m_threaded = true;
        m_error_handler = new ErrorHandler("Process");
        m_environment = top_env;
        m_top = m_environment;
        call.m_declaration->accept(this);
        callDeclaration(&call,{});

    }
    //run a function with params
    void runThread(Callable call, Environment* top_env, std::vector<object> args) {
        m_threaded = true;
        m_error_handler = new ErrorHandler("Thread");
        m_environment = top_env;
        m_top = m_environment;
        call.m_declaration->accept(this);
        callDeclaration(&call,args);
    }

    //statements
    object visitImportStatement(ImportStatement *statement){
        if(statement->m_name.original == "SL"){
            createStandardLibrary(m_environment);
        }
        if(statement->m_name.original == "Thread"){
            createThreadingLibrary(m_environment);
        }
        if(statement->m_name.original == "Random"){
            createRandomLibrary(m_environment);
        }
        return null_object();
    }



    object visitClassStatement(ClassStatement *statement) {
        Class new_class(statement->m_name.original, new Environment(m_environment));
        executeBlock(statement->m_members,new_class.m_environment, false);
        m_environment->define(statement->m_name.original,new_class);
        return null_object();
    }
        object visitFunctionStatement(FunctionStatement *statement) {
        Callable function =  Callable(statement->m_parameters.size(), nullptr,statement);
        m_environment->define(statement->m_name.original,function);
        return null_object();
    }
    object visitIfStatement(IfStatement* statement){
        bool condition = isTruthy(eval(statement->m_condition));
        object out = null_object();
        if(condition){
            out = statement->m_then_branch->accept(this);
        }
        if(statement->m_else_branch != nullptr && !condition){
           out =  statement->m_else_branch->accept(this);
        }
        return out;
    }

    object visitWhileStatement(WhileStatement* statement){
        object out = null_object();
        while(isTruthy(eval(statement->m_condition))){
            out = statement->m_body->accept(this);
            if(std::get_if<Signal>(&out)){
                return null_object();
            }
            if(!std::get_if<null_object>(&out)){
                break;
            }
            //could also use goto rather than c loop,
            //but it ends up the exact same
        }
        return out;
    }

    object visitReturnStatement(ReturnStatement *statement){
        object value = null_object();
        if(statement->m_keyword.original == "break"){
            return Signal(0);
        }
        if(statement->m_value!= nullptr){
            value = eval(statement->m_value);
        }
        return value;
    }

    object visitBlockStatement(BlockStatement* statement){
        return executeBlock(statement->m_statements, new Environment(m_environment));
    }
    object visitVariableStatement(VariableStatement* statement){
        object value = null_object();
        if(statement->m_initializer != nullptr){
            value = eval(statement->m_initializer);
        }
        if(m_environment->define(statement->m_name.original,value,statement->m_constant)){m_error_handler->error("Variable undefined: " + statement->m_name.original);};
        return null_object();
    }

    object visitExpressionStatement(ExpressionStatement* statement){
        eval(statement->m_expression);
        return null_object();
    }


    //expressions
    object visitCallExpression(Call* expression) {
        object callee = eval(expression->m_callee);
        std::vector<object> arguments;
        for (Expression* argument : expression->m_arguments) {
            arguments.push_back(eval(argument));
        }

        //if class
        if(ClassObject* obj = std::get_if<ClassObject>(&callee)){
            //get and run constructor if available
            object constructor = obj->m_environment->getValue(obj->m_class.m_name);
            if(Callable* constructor_function = std::get_if<Callable>(&constructor)){
                m_last_class = *obj;
                callDeclaration(constructor_function,arguments);}
            return callee; //return object
        }

        if(Callable* function = std::get_if<Callable>(&callee)){
            if (arguments.size() != function->m_argument_number && function->m_argument_number > -1) {
                m_error_handler->error("Too many function arguments: " + std::to_string(arguments.size()) + " expected: " +
                                               std::to_string(function->m_argument_number));}
            return callDeclaration(function,arguments);}
        m_error_handler->error(expression->m_line, "Can not call non callable.");
        return null_object();
    }

    object visitLogicExpression(Logic *expression){
        object left = eval(expression->m_left);
        if(expression->m_operator_.type == OR){
            if(isTruthy(left)){ return left;}
        }else{
            if(!isTruthy(left)){return left;}
        }
        return eval(expression->m_right);
    }

    object visitAssignExpression(Assign* expression){
        object value = eval(expression->m_value);

        if(m_environment->isConst(expression->m_name.original)){
            m_error_handler->error(expression->m_line,"Attempt to write constant.");
        }
        else if(expression->m_pointer){
            Reference ref = std::get<Reference>(m_environment->getValue(expression->m_name.original));
            if(!ref.m_env->assign(ref.m_name,value)){m_error_handler->error(expression->m_line,"Pointer undefined: " + expression->m_name.original);};
        }
        else if(!m_environment->assign(expression->m_name.original,value)){m_error_handler->error(expression->m_line,"Assignment undefined: " + expression->m_name.original);};
        return null_object();
    }
    object visitPointerExpression(Pointer *expression){
        object original_value = m_environment->getValue(expression->m_variable->m_name.original);
        if(Reference* ptr = std::get_if<Reference>(&original_value)){
            return ptr->m_env->getValue(ptr->m_name);
        }
        return Reference(expression->m_variable->m_name.original, m_environment->getScope(expression->m_variable->m_name.original));
    }
    object visitVariableExpression(Variable* expression){
        object value =  m_environment->getValue(expression->m_name.original);
        if(Class* class_type = std::get_if<Class>(&value)){
            ClassObject obj =  ClassObject(*class_type, class_type->m_environment->copy());
            return obj;
        }
        return value;
    }
    object visitGetExpression(Get *expression) {
        object value = eval(expression->m_object);
        if (ClassObject* obj = std::get_if<ClassObject>(&value)) {
            m_last_class = *obj;
            return obj->m_environment->getValue(expression->m_name.original);
        }
        m_error_handler->error(expression->m_line,"Cannot read property of non object.");
        return null_object();
    }

    object visitSetExpression(Set *expression) {
        object left = eval(expression->m_obj);
        if(ClassObject* obj = std::get_if<ClassObject>(&left)){
            object right = eval(expression->m_value);
             obj->m_environment->assign(expression->m_name.original, right);
            return null_object();
        }
        m_error_handler->error(expression->m_line,"Cannot write property of non object.");
        return null_object();
    }


    object visitBinaryExpression(Binary* expression){
        object right = eval(expression->m_right);
        object left = eval(expression->m_left);
        //TODO add operator overloading
        switch (expression->m_operator_.type) {
            case BANG_EQUAL: return !isEqual(left, right);
            case EQUAL_EQUAL: return isEqual(left, right);
            case GREATER:
                if(float* number = std::get_if<float>(&left)){return *number > getFloat(right, expression->m_line);}
                if(int* number = std::get_if<int>(&left)){return *number > getInt(right, expression->m_line);}
            case GREATER_EQUAL:
                if(float* number =std::get_if<float>(&left)){return *number >= getFloat(right, expression->m_line);}
                if(int* number =std::get_if<int>(&left)){return *number >= getInt(right, expression->m_line);}
            case LESS:
                if(float* number =std::get_if<float>(&left)){return *number < getFloat(right, expression->m_line);}
                if(int* number =std::get_if<int>(&left)){return *number < getInt(right, expression->m_line);}
            case LESS_EQUAL:
                if(float* number =std::get_if<float>(&left)){return *number <= getFloat(right, expression->m_line);}
                if(int* number =std::get_if<int>(&left)){return *number  <=  getInt(right, expression->m_line);}
            case MINUS:
                if(float* number =std::get_if<float>(&left)){return *number - getFloat(right, expression->m_line);}
                if(int* number =std::get_if<int>(&left)){return *number - getInt(right, expression->m_line);}
            case SLASH:
                if(getFloat(right, expression->m_line) == 0){  m_error_handler->error(expression->m_line,"Divide by zero."); return null_object();}
                if(float* number =std::get_if<float>(&left)){return *number / getFloat(right, expression->m_line);}
                if(int* number =std::get_if<int>(&left)){return *number / getInt(right, expression->m_line);}
            case STAR:
                if(float* number =std::get_if<float>(&left)){return *number * getFloat(right, expression->m_line);}
                if(int* number =std::get_if<int>(&left)){return *number * getInt(right, expression->m_line);}
            case PLUS:
                if(int* number =std::get_if<int>(&left)){return *number + getInt(right, expression->m_line);}
                if(float* number =std::get_if<float>(&left)){return *number + getFloat(right, expression->m_line);}
                if(std::string* s =std::get_if<std::string>(&left)){return *s + getString(right, expression->m_line);}
        }
        m_error_handler->error(expression->m_line,"Not a binary type for: " + expression->m_operator_.original);
        return null_object();
    };
    object visitGroupingExpression(Grouping* expression){
        return eval(expression->m_expression);
    };
    object visitLiteralExpression(Literal* expression){
        return expression->m_value;
    };
    object visitUnaryExpression(Unary* expression){
        object right = eval(expression->m_right);
        switch (expression->m_operator_.type) {
            case BANG:
                return !isTruthy(right);
            case MINUS:
                if(float* number =std::get_if<float>(&right)){return -*number;}
                if(int* number =std::get_if<int>(&right)){return -*number;}
        }
        m_error_handler->error(expression->m_line,"Not a unary type.");
        return null_object();
    };
private:
    Environment* m_environment;
    Environment* m_top;
    ErrorHandler* m_error_handler;
    //last used class object for function scoping
    ClassObject m_last_class{ Class("")};
    object eval(Expression* expression){
        return expression->accept(this);
    }
    object executeBlock(statementList statements, Environment* environment, bool cleanup = true){
        Environment* pre = m_environment;
        m_environment = environment;
        object out = null_object();
        for(Statement* inner_statement: statements){
             out = inner_statement->accept(this);
            if(!std::get_if<null_object>(&out)){
                break;
            }
        }
        if( cleanup){
            delete environment;
        }

        m_environment = pre;
        return out;
    }
     object callDeclaration(Callable* function, std::vector<object> arguments){
        FunctionStatement* declaration = function->m_declaration;
        if(declaration == nullptr){
            return function->m_call(this,arguments);
        }

        //set up scope
         Environment* environment;
        if(m_last_class.m_class.m_name != ""){
            environment = new Environment(m_last_class.m_environment);
            //add this variable
            environment->define("this",m_last_class);
        }else{
            environment = new Environment(m_top);
        }
        for (int i = 0; i < declaration->m_parameters.size(); i++) {
            environment->define(declaration->m_parameters[i]->m_name.original,
                               arguments[i]);
        }
       return executeBlock(declaration->m_body, environment);

    };


    //conversions
    bool isTruthy(object in){
        if(std::get_if<null_object>(&in)) {return false;};
        if(bool* boolean =std::get_if<bool>(&in)){return *boolean;}
        if(int* int_boolean =std::get_if<int>(&in)){return *int_boolean == 1;}
        return true;
    }
    bool isEqual(object a,object b){
        if(std::get_if<null_object>(&a) && std::get_if<null_object>(&b)){return  true;};
        if(std::get_if<null_object>(&a)){return false;};
        if(float* number =std::get_if<float>(&a)){return *number == getFloat(b);}
        if(int* number =std::get_if<int>(&a)){return *number == getInt(b);}
        if(std::string* s =std::get_if<std::string>(&a)){return *s == getString(b);}
        return isTruthy(a) == isTruthy(b);
    }
    int getInt(object in, int line = 0){
        if(int* number =std::get_if<int>(&in)){return *number;}
        if(float* number =std::get_if<float>(&in)){return *number;}
        if(std::string* s =std::get_if<std::string>(&in)){return std::stoi(*s);}
        m_error_handler->error(line, "Could not convert to int.");
        return 0;
    }
    float getFloat(object in, int line = 0){
        if(float* number =std::get_if<float>(&in)){return *number;}
        if(int* number =std::get_if<int>(&in)){return *number;}
        if(std::string* s =std::get_if<std::string>(&in)){return std::stof(*s);}
        m_error_handler->error(line,"Could not convert to float.");
        return 0;
    }
    std::string getString(object in, int line = 0){
        if(std::string* s =std::get_if<std::string>(&in)){return *s;}
        if(float* number =std::get_if<float>(&in)){return std::to_string(*number);}
        if(int* number =std::get_if<int>(&in)){return std::to_string(*number);}
        if(bool* b =std::get_if<bool>(&in)){return std::to_string(*b);}
        if(ClassObject* b =std::get_if<ClassObject>(&in)){return b->m_class.m_name + " object";}
        if(std::get_if<null_object>(&in)){ return "null";};
        m_error_handler->error(line,"Cannot convert to string.");
        return "";
    }







//Standard Library
    static void createStandardLibrary(Environment* env){
        env->define("language",  Callable(0,[](Interpreter* runtime, std::vector<object> args) {
            return object(std::string("DogeScript " + std::string(DOGE_LANGUAGE_VERSION)));
        }));

        //useful function
        env->define("poop",  Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            std::string a = runtime->getString(args[0]);
            return object(std::string("This " + a + " is poop"));
        }));

        env->define("print",  Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            std::string out = runtime->getString(args[0]);
            std::cout << out << "\n";
            return object(null_object());
        }));

        env->define("printSameLine",  Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            std::string out = runtime->getString(args[0]);
            std::cout << out;
            return object(null_object());
        }));

        env->define("input",  Callable(0,[](Interpreter* runtime, std::vector<object> args) {
            std::string input = "";
            std::cin >> input;
            return object(input);
        }));

        env->define("toInt",  Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            return object(runtime->getInt(args[0]));
        }));

        env->define("seconds",  Callable(0,[](Interpreter* runtime, std::vector<object> args) {
            auto  time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            return object((float)time/ 1000.0f);
        }));
    }

    //library for multi threading
    static void createThreadingLibrary(Environment* env){
        //keep track of threads
        static std::vector<std::thread> running;

        //run a standalone function in new thread. Effectively detach
        env->define("fork",  Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            Callable to_run = std::get<Callable>(args[0]);
            Interpreter new_runtime;
            running.push_back(std::thread{ &Interpreter::runForked,new_runtime ,to_run,runtime->m_top->copy()});
            return object((null_object()));
        }));
        //run a thread. Returns thread id.
        env->define("thread",  Callable(-1,[](Interpreter* runtime, std::vector<object> args) {        //-1 args means any number of arguments
            Callable to_run = std::get<Callable>(args[0]);   //get function
            Interpreter new_runtime; //create thread runtime
            //create function args
            std::vector<object> new_args = args;
            new_args.erase(new_args.begin());   //same as thread call args except first
            //keep track of thread
            running.push_back(std::thread{ &Interpreter::runThread,new_runtime ,to_run,runtime->m_top->copy(), new_args});
            return object((int)running.size()-1);
        }));

        //Join equivalent sync from id
        env->define("getThread",Callable(1,[](Interpreter* runtime, std::vector<object> args) {
           int id = runtime->getInt(args[0]);
           running[id].join();
           running.erase(running.begin() + id);
            return object(null_object());
        }));

        //wait for milliseconds
        env->define("waitM",Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            int time = runtime->getInt(args[0]);
            std::this_thread::sleep_for(std::chrono::milliseconds (time));
            return object(null_object());
        }));

        //wait for seconds
        env->define("waitS",Callable(1,[](Interpreter* runtime, std::vector<object> args) {
            int time = runtime->getInt(args[0]);
            std::this_thread::sleep_for(std::chrono::seconds(time));
            return object(null_object());
        }));

        //wait for threads to finish. returns number of threads synced.
        env->define("sync",Callable(0,[](Interpreter* runtime, std::vector<object> args) {
            int number = running.size();
            for(std::thread& runner : running){
                runner.join();
            }
            running.clear();
            return object(number);
        }));

    }
    //random lib
    static void createRandomLibrary(Environment* env){

        env->define("RandomFloat",  Callable(0,[](Interpreter* runtime, std::vector<object> args) {
            float random = rand();
            return object((random));
        }));

        env->define("RandomInt",  Callable(2,[](Interpreter* runtime, std::vector<object> args) {
            int min = runtime->getInt(args[0]);
            int max = runtime->getInt(args[1]);
            int random = rand()%(max-min + 1) + min;
            return object(random);
        }));


    }
};










#endif PROGRAM_INTERPRETER_HPP