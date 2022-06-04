//
// Created by Philip on 6/2/2022.
//

#ifndef PROGRAM_IRCOMPILER_HPP
#define PROGRAM_IRCOMPILER_HPP

#include "Environment.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
class IRCompiler : public Visitor{
public:


    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder {m_context};
    llvm::Module m_module {"Doge Language", m_context};
    std::vector<llvm::Function*> m_assembly;

    //run the interpreter
    void compile(statementList statements,ErrorHandler* error_handler) {
        //setup error handler
        m_error_handler = error_handler;
        m_error_handler->m_name = "IR Compiler";
        m_visitor_name = "IR compiler";

        m_environment = new Environment();
        m_top = m_environment;


        for(Statement* statement:statements){
            object out = statement->accept(this);
            if(llvm::Function** function = std::get_if<llvm::Function*>(&out)){
                m_assembly .push_back(*function);
            }

        }

    }
    void print(){
       for(llvm::Function* function : m_assembly){
           function->print(llvm::outs());
       }
    }

    void optimize(){
        // Create a new pass manager attached to it.
        llvm::legacy::FunctionPassManager optimizer(&m_module);
        // simple optimizations
      optimizer.add(llvm::createInstructionCombiningPass());
        // Associate expressions.
        optimizer.add(llvm::createReassociatePass());
        // Eliminate common Sub Expressions.
        optimizer.add(llvm::createGVNPass());
        // Simplify the control flow graph
        optimizer.add(llvm::createCFGSimplificationPass());

        // Promote allocas to registers.
        optimizer.add(llvm::createPromoteMemoryToRegisterPass());
// Do simple "peephole" optimizations and bit-twiddling optzns.
        optimizer.add(llvm::createInstructionCombiningPass());
// Reassociate expressions.
        optimizer.add(llvm::createReassociatePass());
        optimizer.doInitialization();

        for(llvm::Function* function : m_assembly){
            optimizer.run(*function);
        }
    }
    void output() {

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();


        auto TargetTriple = llvm::sys::getDefaultTargetTriple();

        std::string Error;
        auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
        if (!Target) {
            llvm::errs() << Error;
            return;
        }


        auto CPU = "generic";
        auto Features = "";

        llvm::TargetOptions opt;
        auto RM = llvm::Optional<llvm::Reloc::Model>();
        auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
        m_module.setDataLayout(TargetMachine->createDataLayout());
        m_module.setTargetTriple(TargetTriple);

        auto Filename = "output.o";
        std::error_code EC;
        llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

        if (EC) {
            llvm::errs() << "Could not open file: " << EC.message();
            return;
        }


        llvm::legacy::PassManager pass;
        auto FileType =  llvm::CodeGenFileType::CGFT_ObjectFile;

        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            llvm::errs() << "TargetMachine can't emit a file of this type";
            return;
        }

        pass.run(m_module);
        dest.flush();
        dest.close();

        std::cout << "\n Assembling... \n";
        system("clang++ output.cpp output.o -o output.exe");

        std::cout << "\n Running... \n";
        system("output.exe");
    }


    object visitWhileStatement(WhileStatement* statement){


        llvm::Function *parent = m_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *loop_block = llvm::BasicBlock::Create(m_context, "loop", parent);
        m_builder.CreateBr(loop_block);
        m_builder.SetInsertPoint(loop_block);
        object out = statement->m_body->accept(this);

        llvm::Value* condition = std::get<llvm::Value*>(eval(statement->m_condition));

        llvm::BasicBlock *after_block =
                llvm::BasicBlock::Create(m_context, "after_loop", parent);

        m_builder.CreateCondBr(condition, loop_block, after_block);
        m_builder.SetInsertPoint(after_block);
        return null_object();
    }
    object visitAssignExpression(Assign *expression){
        llvm::Value* value = std::get<llvm::Value*>(eval(expression->m_value));
        llvm::Value *Variable =std::get<llvm::AllocaInst*>(m_environment->getValue(expression->m_name.original));
        m_builder.CreateStore(value, Variable);
        return null_object();
    }


    //statements
    object visitIfStatement(IfStatement* statement){
        llvm::Value* condition = std::get<llvm::Value*>(eval(statement->m_condition));

        llvm::Function *parent = m_builder.GetInsertBlock()->getParent();


        llvm::BasicBlock *then_block = llvm::BasicBlock::Create(m_context, "if_then", parent);
        llvm::BasicBlock *else_block = llvm::BasicBlock::Create(m_context, "if_else");
        llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(m_context, "if_after");

        m_builder.CreateCondBr(condition, then_block, else_block);

        m_builder.SetInsertPoint(then_block);





        object then_out  = statement->m_then_branch->accept(this);


        m_builder.CreateBr(merge_block);
        then_block = m_builder.GetInsertBlock();

        parent->getBasicBlockList().push_back(else_block);
        m_builder.SetInsertPoint(else_block);



        if(statement->m_else_branch != nullptr){
            object else_out = statement->m_else_branch->accept(this);
        }


        m_builder.CreateBr(merge_block);
        else_block = m_builder.GetInsertBlock();

        // Emit merge block.
        parent->getBasicBlockList().push_back(merge_block);
        m_builder.SetInsertPoint(merge_block);


        return null_object();
    }
    object visitFunctionStatement(FunctionStatement *statement) {
        llvm::Function *function = m_module.getFunction(statement->m_name.original);
        Callable callable =  Callable(statement->m_parameters.size(), nullptr,statement);
        m_environment->define(statement->m_name.original,callable);

        if (function){
            return function;
        }


        //TODO set parameter and return types in parser
        std::vector<llvm::Type*> types(statement->m_parameters.size(),
                                   llvm::Type::getFloatTy(m_context));
        //TODO remove this temporary fix once types are in
        if(statement->m_name.original == "printC"){
           types = std::vector<llvm::Type*>(statement->m_parameters.size(),
                                            llvm::Type::getInt8PtrTy(m_context));

        }

        llvm::FunctionType *function_type = llvm::FunctionType::get(llvm::Type::getFloatTy(m_context), types, false);
        function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, statement->m_name.original, m_module);
        //set names
        int x = 0;
        for (llvm::Argument& argument : function->args()){
            argument.setName(statement->m_parameters[x++].original);
        }

        if(statement->m_body.empty()){
            return function;
        }

        llvm::BasicBlock *block = llvm::BasicBlock::Create(m_context, "entry", function);
        m_builder.SetInsertPoint(block);
        //set up scope
        Environment* environment;
        environment = new Environment(m_top);

        for (auto &Arg : function->args()){

            llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(function, std::string(Arg.getName()));

            // Store the initial value into the alloca.
            m_builder.CreateStore(&Arg, Alloca);

            environment->define(std::string(Arg.getName()),  Alloca);
        }


        object out = executeBlock(statement->m_body, environment);





            // Validate the generated code, checking for consistency.
            llvm::verifyFunction(*function);

        return function;
    }


    object visitReturnStatement(ReturnStatement *statement){


        if(statement->m_value!= nullptr){
            object value = eval(statement->m_value);
            m_builder.CreateRet(std::get<llvm::Value*>(value));
        }else{
            m_builder.CreateRetVoid();
        }


        return null_object();
    }

    object visitBlockStatement(BlockStatement* statement){
        return executeBlock(statement->m_statements, new Environment(m_environment));
    }
    object visitVariableStatement(VariableStatement* statement){
        llvm::Function *parent = m_builder.GetInsertBlock()->getParent();
        llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(parent, statement->m_name.original);

        m_environment->define(std::string(statement->m_name.original),  Alloca);

        llvm::Value* value =  (llvm::Value*)llvm::ConstantFP::get(m_context, llvm::APFloat(0.0f));

        if(statement->m_initializer != nullptr){
            value = std::get<llvm::Value*>(eval(statement->m_initializer));
        }
        m_builder.CreateStore(value, Alloca);


        return null_object();
    }

    object visitExpressionStatement(ExpressionStatement* statement){
        eval(statement->m_expression);
        return null_object();
    }


    //expressions
    object visitCallExpression(Call* expression) {
        // Look up the name in the global module table.
        Callable callee = std::get<Callable> (eval(expression->m_callee));
        llvm::Function *callee_function = m_module.getFunction(callee.m_declaration->m_name.original);
        if (!callee_function){
            m_error_handler->error(expression->m_line,"Unknown function: " + callee.m_declaration->m_name.original);
            return 0;
        }
        if (callee_function->arg_size() != expression->m_arguments.size()){
            m_error_handler->error(expression->m_line,"Not correct argument number, function:  " + callee.m_declaration->m_name.original + " expects: " +
                    std::to_string(callee_function->arg_size()) + " not " + std::to_string(expression->m_arguments.size()));
            return 0;
        }
        std::vector<llvm::Value* > arguments;
        for (Expression* argument : expression->m_arguments) {
            arguments.push_back(std::get<llvm::Value*>(eval(argument)));
        }
        return (llvm::Value* )m_builder.CreateCall(callee_function, arguments, "call");
    }




    object visitVariableExpression(Variable* expression){
        object in =  m_environment->getValue(expression->m_name.original);

        if( llvm::AllocaInst** val = std::get_if< llvm::AllocaInst* >(&in)){
            llvm::AllocaInst* value = *val;
            return (llvm::Value*)m_builder.CreateLoad(value->getAllocatedType(),value, expression->m_name.original.c_str());

        }else{
            return in;
        }

    }





    object visitBinaryExpression(Binary* expression){
        llvm::Value* right = std::get<llvm::Value*>(eval(expression->m_right));
        llvm::Value* left =  std::get<llvm::Value*>(eval(expression->m_left));
        //TODO add operator overloading
        switch (expression->m_operator_.type) {
            case BANG_EQUAL:
                return 0;
            case EQUAL_EQUAL:
                return 0;
            case GREATER:
                return 0;
            case GREATER_EQUAL:
                return 0;
            case LESS:
                if(left->getType()->isFloatTy() && right->getType()->isFloatTy()){
                    return m_builder.CreateFCmpULT(left, right, "lessFloat");
                }else{
                    return m_builder.CreateICmpULT(left, right, "lessInt");
                }
            case LESS_EQUAL:
                return 0;
            case MINUS:
                if(right->getType()->isFloatTy()){
                    return m_builder.CreateFSub(left, right, "subFloat");
                }else{
                    return m_builder.CreateSub(left, right, "subInt");
                }
            case SLASH:
                //check if zero
                if(right->getType()->isFloatTy()){
                    return m_builder.CreateFDiv(left, right, "divFloat");
                }else{
                    return m_builder.CreateUDiv(left, right, "divInt");
                }
            case STAR:
                if(right->getType()->isFloatTy()){
                    return m_builder.CreateFMul(left, right, "mulFloat");
                }else{
                    return m_builder.CreateMul(left, right, "mulInt");
                }
            case PLUS:
                if(right->getType()->isFloatTy()){
                    return m_builder.CreateFAdd(left, right, "addFloat");
                }else{
                    return m_builder.CreateAdd(left, right, "addInt");
                }

        }
        m_error_handler->error(expression->m_line,"Not a binary type for: " + expression->m_operator_.original);
        return 0;
    };

    object visitGroupingExpression(Grouping* expression){
        return eval(expression->m_expression);
    };
    object visitLiteralExpression(Literal* expression){
        if(int* number =std::get_if<int>(& expression->m_value)){return llvm::ConstantInt::get(m_context, llvm::APInt(sizeof(int),*number));}
        if(float* number =std::get_if<float>(& expression->m_value)){return llvm::ConstantFP::get(m_context, llvm::APFloat(*number));}
        if(bool* boolean =std::get_if<bool>(& expression->m_value)){return llvm::ConstantInt::get(m_context, llvm::APInt(sizeof(int),(int)*boolean));}
     //   if(std::string* s =std::get_if<std::string>(& expression->m_value)){return llvm::ConstantFP::get(m_context, llvm::APFloat((float)(s->at(0))));}

        if(std::string* s =std::get_if<std::string>(& expression->m_value)){
            return (llvm::Value*)m_builder.CreateGlobalStringPtr(*s);

        }
        return 0;
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

     llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction,
                                              const std::string &VarName) {
        llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                         TheFunction->getEntryBlock().begin());
        return TmpB.CreateAlloca(llvm::Type::getFloatTy(m_context), 0,
                                 VarName.c_str());
    }

    //conversions
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





    };
#endif //PROGRAM_IRCOMPILER_HPP
