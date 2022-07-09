//
// Created by Philip on 6/2/2022.
//

#ifndef PROGRAM_IRCOMPILER_HPP
#define PROGRAM_IRCOMPILER_HPP

#include <filesystem>
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

//compile AST to llvm intermediate representation
class IRCompiler : public Visitor {
public:
    //keep track of file names for errors
    std::vector<std::string> m_names;
    //run the interpreter
    void compile(const statementList& statements, std::map<std::string,statementList> external_files, ErrorHandler *error_handler, const std::string& file) {
        //setup error handler
        m_error_handler = error_handler;
        m_error_handler->m_name = "IR Compiler";
        m_error_handler->m_file = file;
        m_visitor_name = "IR compiler";
        m_external_files = external_files;


        //create top level env
        m_environment = new Environment();
        m_top = m_environment;

        m_names.push_back(file);
        //run each statement
        for (Statement *statement: statements) {
            statement->accept(this);
        }
        m_names.pop_back();
    }

    //run optimizations on llvm ir
    void optimize() {
        //create pass
        llvm::legacy::PassManager optimizer;
        //add passes and run
        optimizer.add(llvm::createPromoteMemoryToRegisterPass());
        optimizer.add(llvm::createInstructionCombiningPass());
        optimizer.add(llvm::createReassociatePass());
        optimizer.add(llvm::createGVNPass());
        optimizer.run(m_module);
    }

    void print() {
        m_module.print(llvm::outs(), nullptr);
    }

    void printFile(const std::string& filename) {
        //create output file
        std::error_code stream_error;
        llvm::raw_fd_ostream dest(filename, stream_error, llvm::sys::fs::OF_None);
        if (stream_error) {
            llvm::errs() << "Could not write to file: " << stream_error.message();
            return;
        }
        m_module.print(dest, nullptr);
        //close file
        dest.flush();
        dest.close();
    }

    //Create object file
    void build(const std::string& out_name) {
        //initialize
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
        //get platform target
        auto desired_target = llvm::sys::getDefaultTargetTriple();
        std::string error;
        auto target = llvm::TargetRegistry::lookupTarget(desired_target, error);
        if (!target) {
            llvm::errs() << error;
            return;
        }

        //create device settings
        auto cpu = "generic";
        auto features = "";

        //create target
        llvm::TargetOptions target_options;
        auto target_machine = target->createTargetMachine(desired_target, cpu, features, target_options,
                                                          llvm::Optional<llvm::Reloc::Model>());
        //setup module
        m_module.setDataLayout(target_machine->createDataLayout());
        m_module.setTargetTriple(desired_target);

        //create output file
        std::string filename = out_name;
        std::filesystem::path file_path = out_name;
        m_object_filename = out_name;
        auto file_type = llvm::CodeGenFileType::CGFT_ObjectFile;
        if(file_path.extension() == ".s") {
            file_type =  llvm::CodeGenFileType::CGFT_AssemblyFile;
        }else if(file_path.extension() != ".o"){
            llvm::errs() << "Can only output object or assembly file.";
            return;
        }

        std::error_code stream_error;
        llvm::raw_fd_ostream dest(filename, stream_error, llvm::sys::fs::OF_None);
        if (stream_error) {
            llvm::errs() << "Could not write to file: " << stream_error.message();
            return;
        }

        //create output_pass
        llvm::legacy::PassManager output_pass;
        if (target_machine->addPassesToEmitFile(output_pass, dest, nullptr, file_type)) {
            llvm::errs() << "Target cannot create file.";
            return;
        }
        output_pass.run(m_module);
        //close file
        dest.flush();
        dest.close();
    }

    bool assemble(const std::string& out_name,std::string vcvarsall_location) {
        //assemble file to executable using clang
        //add external cpp or .o files
        std::string dependencies;
        for(std::string external : externals){dependencies = dependencies + " " + external;}
        std::cout << "\n linking files: " + dependencies + " " + m_object_filename << "\n";
        //check which compilers work for assembling and linking
        if(system(("clang++" + dependencies + " " + m_object_filename + " -o " + out_name).c_str())){
            Color::start(YELLOW);std::cout << "Error with clang, trying gcc \n";Color::end();
            if(system(("g++" + dependencies + " " + m_object_filename + " -o " + out_name).c_str())) {
                Color::start(YELLOW);std::cout << "Error with gcc, trying msvc \n"; Color::end();
                if(vcvarsall_location.empty()){
                    Color::start(RED);std::cout << "\n \n Please define vcvarsall.bat folder location for msvc using -m!!!! \n";Color::end();
                    std::cout << "Alternatively run this in your visual studio dev prompt: " << "cl" + dependencies + " " + m_object_filename + " /Fe: " + out_name << "\n";
                    return false;
                }
                std::filesystem::path current = std::filesystem::current_path();
                std::cout << "going to: " << vcvarsall_location << "\n";
                std::cout << "Running " << "vcvarsall.bat x64" << "\n";
                std::cout << "going to: " << current << "\n";
                std::cout << "Running " << "cl" + dependencies + " " + m_object_filename + " /Fe: " + out_name << "\n";
                if(system(("cd " + vcvarsall_location + " && vcvarsall.bat x64 && cd " + current.string() + " && cl" + dependencies + " " + m_object_filename + " /Fe: " + out_name).c_str())) {
                    Color::start(RED);std::cout << "Error with msvc.\n";Color::end();
                    return false;
                }else{
                    Color::start(GREEN);std::cout << "Successfully linked using msvc! \n";Color::end();
                    return true;
                }

            }else{
                Color::start(GREEN);std::cout << "Successfully linked using gcc! \n";Color::end();
                return true;
            }
        }else{
            Color::start(GREEN);std::cout << "Successfully linked using clang! \n";Color::end();
            return true;
        }
    }

    //statements

    //import code from other files
    object visitImportStatement(ImportStatement *statement) {
        statementList statements = m_external_files[statement->m_name.original];
        m_names.push_back(statement->m_name.original);
        m_error_handler->m_file = m_names.back();
        //run each statement
        for (Statement *new_statement: statements) {
            new_statement->accept(this);
        }
        m_names.pop_back();
        m_error_handler->m_file = m_names.back();
        return null_object();
    }
    object visitIncludeStatement(IncludeStatement *statement) {
        if(statement->m_link){
            //set .o or .cpp files to be linked when made executable
            externals.push_back(std::get<std::string>(statement->m_name.value));
            return null_object();
        }
        statementList statements = m_external_files[std::get<std::string>(statement->m_name.value)];
        //run each statement
        m_names.push_back(statement->m_name.original);
        m_error_handler->m_file = m_names.back();
        for (Statement *new_statement: statements) {
            new_statement->accept(this);
        }
        m_names.pop_back();
        m_error_handler->m_file = m_names.back();
        return null_object();
    }

    object visitClassStatement(ClassStatement *statement) {
        //create new type
        llvm::StructType *new_class = llvm::StructType::create(m_context, statement->m_name.original);
        //define class
        m_environment->define(statement->m_name.original + "_class", (llvm::Type *) new_class);
        //member variable types
        std::vector<llvm::Type *> types;
        //constructor and destructor statements
        std::vector<Statement *> inits;
        std::vector<Statement *> destructs;
        int id = 0;
        for (Statement *member: statement->m_members) {
            //if member is variable
            VariableStatement *member_var = dynamic_cast<VariableStatement *>(member);
            if (member_var != nullptr) {
                //add type
                types.push_back(getType(member_var->m_type));
                //define variable
                m_environment->define(statement->m_name.original + "_class_" + member_var->m_name.original, id);
                //if of class type define type
                object class_type = m_environment->getValue(member_var->m_type.original + "_class");
                if (!std::get_if<null_object>(&class_type)) {
                    m_environment->define(statement->m_name.original + "_class_" + member_var->m_name.original + "_type",member_var->m_type.original + "_class");
                    destructs.push_back(new ExpressionStatement(new Assign(member_var, true),statement->m_line));
                }
                //check for member initialization
                if(member_var->m_initializer != nullptr){
                    inits.push_back(new ExpressionStatement(new Assign(member_var),statement->m_line));
                }else if(!std::get_if<null_object>(&class_type)){
                    m_error_handler->warning(member_var->m_line,"Uninitialized member class contains garbage. assign it to a class() to call constructors! Or you will face the wrath of weird destructors!");
                }
                id++; //increment
            }
        }
        //set body
        new_class->setBody(types);
        //create default constructor for initializing members
        if(!inits.empty()){
            FunctionStatement constructor(Token("default"),{},inits,Token("void"),statement->m_line,statement->m_name.original);
            constructor.accept(this);
        }
        //create default destructor
        if(!destructs.empty()){
            FunctionStatement destructor(Token("destructor_default"),{},destructs,Token("void"),statement->m_line,statement->m_name.original);
            destructor.accept(this);
        }

        //add member functions
        for (Statement *member: statement->m_members) {
            if (dynamic_cast<VariableStatement *>(member) == nullptr) {
                //is a function
                member->accept(this);
            }
        }
        return null_object();
    }

    object visitWhileStatement(WhileStatement *statement) {
        //get parent
        llvm::Function *parent = m_builder.GetInsertBlock()->getParent();
        //create block
        llvm::BasicBlock *loop_block = llvm::BasicBlock::Create(m_context, "loop", parent);
        //create insert
        m_builder.CreateBr(loop_block);
        m_builder.SetInsertPoint(loop_block);
        //make body
        statement->m_body->accept(this);
        //get condition
        llvm::Value *condition = std::get<llvm::Value *>(eval(statement->m_condition));
        //merge
        llvm::BasicBlock *merge_block =llvm::BasicBlock::Create(m_context, "after_loop", parent);
        m_builder.CreateCondBr(condition, loop_block, merge_block);
        m_builder.SetInsertPoint(merge_block);

        return null_object();
    }

    object visitIfStatement(IfStatement *statement) {
        //get condition
        llvm::Value *condition = std::get<llvm::Value *>(eval(statement->m_condition));
        //get parent
        llvm::Function *parent = m_builder.GetInsertBlock()->getParent();
        //create blocks
        llvm::BasicBlock *then_block = llvm::BasicBlock::Create(m_context, "if_then", parent);
        llvm::BasicBlock *else_block = llvm::BasicBlock::Create(m_context, "if_else");
        llvm::BasicBlock *merge_block = llvm::BasicBlock::Create(m_context, "if_after");
        //create insert point
        m_builder.CreateCondBr(condition, then_block, else_block);
        m_builder.SetInsertPoint(then_block);
        //make body
        statement->m_then_branch->accept(this);
        //create else insert point
        m_builder.CreateBr(merge_block);
        parent->getBasicBlockList().push_back(else_block);
        m_builder.SetInsertPoint(else_block);
        //make else body
        if (statement->m_else_branch != nullptr) {
            statement->m_else_branch->accept(this);
        }
        //create merge
        m_builder.CreateBr(merge_block);
        parent->getBasicBlockList().push_back(merge_block);
        m_builder.SetInsertPoint(merge_block);

        return null_object();
    }

    object visitFunctionStatement(FunctionStatement *statement) {


        //get basic info
        std::string name = statement->m_name.original;
        int size = statement->m_parameters.size();
        bool is_class_member = false;
        //if class member
        if (statement->m_class_name != "") {
            name = statement->m_class_name + "_class_" + statement->m_name.original;
            size++;
            is_class_member = true;
        }


        //create callable and define
        Callable callable = Callable(size, nullptr, statement);
        callable.name = name;
        callable.m_class = statement->m_class_name;
        m_environment->define(name, callable);

        //add overload name
        for (VariableStatement *param: statement->m_parameters) {
            name = name + ("_" + param->m_type.original);
        }
        callable.name = name;
        m_environment->define(name, callable);

        //check if function exists
        llvm::Function *function = m_module.getFunction(name);
        if (function) {return function;}

        //add types
        std::vector<llvm::Type *> types;
        for (VariableStatement *param: statement->m_parameters) {
            types.push_back(getType(param->m_type));
        }
        //add class as this parameter pointer
        if (is_class_member) {
            llvm::Type *class_type = std::get<llvm::Type *>(m_environment->getValue(statement->m_class_name + "_class"));
            types.push_back(llvm::PointerType::get(class_type, 0));
        }

        //create function
        llvm::FunctionType *function_type = llvm::FunctionType::get(getType(statement->m_type), types, false);
        function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, name, m_module);

        //set names
        int x = 0;
        for (llvm::Argument &argument: function->args()) {
            if (x == function->arg_size() - 1 && is_class_member) {
                argument.setName("this");
            } else {
                argument.setName(statement->m_parameters[x++]->m_name.original);
            }
        }
        //create body
        if (statement->m_body.empty()) {return function;} //is extern
        llvm::BasicBlock *block = llvm::BasicBlock::Create(m_context, "entry", function);
        m_builder.SetInsertPoint(block);
        //set up scope
        Environment *environment;
        environment = new Environment(m_top);

        if (is_class_member) {
            environment->define("this_type", statement->m_class_name + "_class");
        }
        //add arguments
        for (auto &Arg: function->args()) {
            llvm::AllocaInst *Alloca = blockAllocation(function, std::string(Arg.getName()), Arg.getType());
            // Store the initial value into the alloca.
            m_builder.CreateStore(&Arg, Alloca);
            environment->define(std::string(Arg.getName()), Alloca);
            //define class types if of class type
            if (Arg.getType()->isStructTy()) {
                object class_type = m_environment->getValue(Arg.getType()->getStructName().str() + "_class");
                if (llvm::Type **class_def = std::get_if<llvm::Type *>(&class_type)) {
                    environment->define(Arg.getName().str() + "_type",Arg.getType()->getStructName().str() + "_class");
                }
            }
            if (Arg.getType()->isPointerTy()) {
                if(Arg.getType()->getContainedType(0)->isStructTy()){
                    object class_type = m_environment->getValue(Arg.getType()->getContainedType(0)->getStructName().str() + "_class");
                    if (llvm::Type **class_def = std::get_if<llvm::Type *>(&class_type)) {
                        environment->define(Arg.getName().str() + "_type",Arg.getType()->getContainedType(0)->getStructName().str() + "_class");
                    }
                }

            }
        }
        //make body
        evalBlock(statement->m_body, environment);
        //make sure a return is present
        if(statement->m_type.original == "void"){
            m_builder.CreateRetVoid();
        }

        //validate
        llvm::verifyFunction(*function);
        return null_object();
    }
    //did return just happen
    bool m_returned = false;
    object visitReturnStatement(ReturnStatement *statement) {
        //destructors
        destruct();
        if (statement->m_value != nullptr) {
            object value = eval(statement->m_value);
            m_builder.CreateRet(std::get<llvm::Value *>(value));
        } else {
            m_builder.CreateRetVoid();
        }
        m_returned = true;
        return null_object();
    }

    object visitBlockStatement(BlockStatement *statement) {
        evalBlock(statement->m_statements, new Environment(m_environment));
        return null_object();
    }

    object visitVariableStatement(VariableStatement *statement) {
        llvm::Function *parent = m_builder.GetInsertBlock()->getParent();
        //still get class type if pointer
        std::string type_name = statement->m_type.original;
        if(type_name.find("_ptr") != std::string::npos) {
            type_name.erase(type_name.length() - 4);
        }
        //check if class
        object class_type = m_environment->getValue(type_name + "_class");
        if (std::get_if<llvm::Type *>(&class_type)) {
            //define type
            m_environment->define(statement->m_name.original + "_type", type_name  + "_class");
        }
        llvm::AllocaInst *variable = blockAllocation(parent, statement->m_name.original, getType(statement->m_type));
        m_environment->define(std::string(statement->m_name.original), variable);
        //initialize
        if (statement->m_initializer != nullptr) {
            llvm::Value *value = std::get<llvm::Value *>(eval(statement->m_initializer));
            //implicit conversion
            if(variable->getAllocatedType()->isFloatTy()){
                value = toFloat(value);
            }
            m_builder.CreateStore(value, variable);
        }else if(variable->getType()->getNonOpaquePointerElementType()->isStructTy()){
            m_error_handler->warning(statement->m_line,"Uninitialized class contains garbage. assign it to a class() to call constructors! Or you will face the wrath of weird destructors!");
        }
        return null_object();
    }

    object visitExpressionStatement(ExpressionStatement *statement) {
        eval(statement->m_expression);
        return null_object();
    }

    //expressions
    object visitGetExpression(Get *expression) {
        //get object
        llvm::Value* class_object =  std::get<llvm::Value *>(eval(expression->m_object));
        llvm::Value *class_object_pointer = m_last_pointer;
        if(class_object->getType()->isPointerTy()){class_object_pointer = class_object;} //if is pointer use value not pointer to pointer
        //get class name
        object class_name_obj = m_environment->getValue(m_last_variable_name + "_type");
        std::string class_name;
        if( std::string* class_name_ptr = std::get_if<std::string >(&class_name_obj)) {
            class_name = *class_name_ptr;
        }else{
            //this is a temp var, so the name of the class will just be the last name used
            //eg string("aaa").print() means the last name is string
            class_name = m_last_variable_name + "_class";
        }
        //get index
        object index_obj = m_environment->getValue(class_name + "_" + expression->m_name.original);
        int index;
        if (int *index_temp = std::get_if<int>(&index_obj)) {
            index = *index_temp;
        } else {
            //return function call
            m_last_variable_name = class_name + "_" + expression->m_name.original;
            m_last_class = class_object_pointer;
            return m_environment->getValue(class_name + "_" + expression->m_name.original);
        }
        //create index array
        std::vector<llvm::Value *> indices = getIndices(index);
        //get class type
        object class_type = m_environment->getValue(class_name);
        if (llvm::Type **class_def = std::get_if<llvm::Type *>(&class_type)) {
            //get member pointer
            llvm::Value *member_ptr = m_builder.CreateGEP(*class_def, class_object_pointer, indices,expression->m_name.original);
            //set variables for next get expression
            m_last_pointer = (llvm::AllocaInst *) member_ptr;
            m_last_variable_name = class_name + "_" + expression->m_name.original;
            m_last_class = class_object_pointer;
            //return value
            return (llvm::Value *) m_builder.CreateLoad(((llvm::StructType *) *class_def)->getElementType(index),member_ptr, expression->m_name.original + "_loaded");
        }
        return null_object();
    }
    object visitSetExpression(Set *expression) {
        //get object
        llvm::Value* class_object =  std::get<llvm::Value *>(eval(expression->m_object));
        llvm::Value *class_object_pointer = m_last_pointer;
        if(class_object->getType()->isPointerTy()){class_object_pointer = class_object;} //if is pointer use value not pointer to pointer
        std::string class_name = std::get<std::string>(m_environment->getValue(m_last_variable_name + "_type"));
        //get index
        object index_obj = m_environment->getValue(class_name + "_" + expression->m_name.original);
        int index;
        if (int *index_temp = std::get_if<int>(&index_obj)) {
            index = *index_temp;
            //create index array
            std::vector<llvm::Value *> indices = getIndices(index);

            //get value to assign
            llvm::Value *value = std::get<llvm::Value *>(eval(expression->m_value));

            //get class type
            object class_type = m_environment->getValue(class_name);
            if (llvm::Type **class_def = std::get_if<llvm::Type *>(&class_type)) {
                //get member pointer
                llvm::Value *member_ptr = m_builder.CreateGEP(*class_def, class_object_pointer, indices,
                                                              expression->m_name.original);
                //set variables for next get expression
                m_last_pointer = (llvm::AllocaInst *) member_ptr;
                m_last_variable_name = class_name + "_" + expression->m_name.original;
                m_last_class = class_object_pointer;

                //implicit conversion
                if(((llvm::StructType *) *class_def)->getElementType(index)->isFloatTy()){
                    value = toFloat(value);
                }
                //store value
                m_builder.CreateStore(value, member_ptr);
            }
        }
        return null_object();
    }



    object visitCallExpression(Call *expression) {
        object callee_temp = eval(expression->m_callee);

        //get arguments
        std::string overload = "";
        std::vector<llvm::Value *> arguments;
        for (Expression *argument: expression->m_arguments) {
            arguments.push_back(std::get<llvm::Value *>(eval(argument)));
            //add overload
            overload = overload + "_" + getTypeName(arguments.back()->getType());
        }

        //is class type. Call class constructor and return new instance
        if(llvm::Type** class_type_temp = std::get_if<llvm::Type*>(&callee_temp)){
            //create object instance
            llvm::Type* class_type = *class_type_temp;
            llvm::Function *parent = m_builder.GetInsertBlock()->getParent();
            llvm::AllocaInst *alloca_inst = blockAllocation(parent, "temp_class",class_type);
            //run member init
            llvm::Function *init = m_module.getFunction(class_type->getStructName().str() + "_class_"+ "default");
            //might not exist if empty. This is to avoid linking error if empty default constructor is optimized away
            if(init){
                m_builder.CreateCall(init, {alloca_inst});
            }

            //get and run constructor if available
            llvm::Function *constructor = m_module.getFunction(class_type->getStructName().str() + "_class_" + class_type->getStructName().str() + overload);
            if (constructor) {
                arguments.push_back(alloca_inst);
                m_builder.CreateCall(constructor, arguments);
            }
            m_last_pointer = alloca_inst;
            //return object
            return (llvm::Value *) m_builder.CreateLoad((llvm::StructType *)class_type,alloca_inst, "temp_class_constructor_loaded");
        }



        // Look up the name in the global module table.
        Callable callee = std::get<Callable>(callee_temp);
        //check for built in array function
        if(callee.name == "array" || callee.name == "local_array"){
            if(expression->m_arguments.size() != 2){  m_error_handler->error(expression->m_line, "Array takes 2 arguments");return null_object();}
            //get type
            llvm::Value *variable = arguments[0];
            //get array size
            llvm::Value* array_size = arguments[1];
            if(array_size->getType() != llvm::Type::getInt32Ty(m_context)){m_error_handler->error(expression->m_line, "Array need int size");return null_object();}

            //allocate
            llvm::Instruction* to_return;
            if(callee.name == "local_array"){

                to_return =  m_builder.CreateAlloca(variable->getType(), array_size);
            }else{
                //get size
                llvm::Constant* AllocSize =   llvm::ConstantExpr::getSizeOf(variable->getType());
                AllocSize = llvm::ConstantExpr::getTruncOrBitCast(AllocSize, llvm::Type::getInt32Ty(m_context));
                //temporary instruction to trick the evil llvm into  thinking its putting its stuff before
                llvm::Instruction* to_move =  m_builder.CreateAlloca(variable->getType(), array_size);
                to_return = llvm::CallInst::CreateMalloc(m_builder.GetInsertPoint()->getPrevNode(),llvm::Type::getInt32Ty(m_context),variable->getType(), AllocSize,
                                                         array_size, nullptr);
                //remove temporary instruction
                to_move->eraseFromParent();

            }
            m_builder.CreateStore(variable,(llvm::Value*)to_return);

            return  (llvm::Value*)to_return;
        }

        std::string name = callee.name + overload;



        llvm::Function *callee_function = m_module.getFunction(name);
        if (!callee_function) {m_error_handler->error(expression->m_line, "Unknown function: " + name);return null_object();}
        //add this
        if (callee.m_class != "") {
            arguments.push_back(m_last_class);
        }

        if (callee_function->arg_size() != arguments.size()) {
            m_error_handler->error(expression->m_line,
                                   "Not correct argument number, function:  " + name +
                                   " expects: " +
                                   std::to_string(callee_function->arg_size()) + " not " +
                                   std::to_string(expression->m_arguments.size()));
            return null_object();
        }
        return (llvm::Value *) m_builder.CreateCall(callee_function, arguments);
    }
    //add this to expression and see if it works. Otherwise, just normally search for variable.
    object visitVariableExpression(Variable *expression) {
        //check for special fucntions
        if(expression->m_name.original == "array"){
            return Callable("array" );
        }else if(expression->m_name.original == "local_array"){
            return Callable("local_array" );
        }
        //check if a member variable matches
        object this_object = m_environment->getValue("this");
        //does this function have a this
        if( llvm::AllocaInst** var = std::get_if<llvm::AllocaInst*>(&this_object)){
            llvm::AllocaInst* this_pointer_pointer = *var;
            //get this
            llvm::Value* this_pointer =  (llvm::Value*)m_builder.CreateLoad(this_pointer_pointer->getAllocatedType(), this_pointer_pointer, "this_load");
            //class name
            std::string class_name = std::get<std::string>(m_environment->getValue("this_type"));
            //member variable exists desired_variable class
            object index_obj = m_environment->getValue(class_name + "_" + expression->m_name.original);
            if(int* index_temp = std::get_if<int>(&index_obj)) {
                //get index
                int index = *index_temp;
                //create index array
                std::vector<llvm::Value *> indices = getIndices(index);
                //get class type
                object class_type =  m_environment->getValue(class_name);
                if (llvm::Type **class_def = std::get_if<llvm::Type *>(&class_type)) {
                    llvm::Value *member_ptr = m_builder.CreateGEP(*class_def, this_pointer, indices,expression->m_name.original);
                    //set variables
                    m_last_pointer = (llvm::AllocaInst *) member_ptr;

                    m_last_variable_name = class_name + "_" + expression->m_name.original;
                    m_last_class = this_pointer;
                    //return load
                    return (llvm::Value *) m_builder.CreateLoad(((llvm::StructType *) *class_def)->getElementType(index),member_ptr, expression->m_name.original + "_loaded");
                }
            }
            //check if member function
            object function_obj = m_environment->getValue(class_name + "_" +  expression->m_name.original);
            if(!std::get_if<null_object>(&function_obj)){
                //is function/callable
                m_last_variable_name = class_name + "_" + expression->m_name.original;
                m_last_class = this_pointer;
                return function_obj;
            }
        }
        //not a member variable, search for local ones
        object desired_variable =  m_environment->getValue(expression->m_name.original);
        //check if pointer exists to variable
        if( llvm::AllocaInst** val = std::get_if< llvm::AllocaInst* >(&desired_variable)){
            llvm::AllocaInst* pointer = *val;
            m_last_pointer = pointer;
            m_last_variable_name = expression->m_name.original;
            //load variable
            return (llvm::Value*)m_builder.CreateLoad(pointer->getAllocatedType(), pointer, expression->m_name.original.c_str());

        }else{
            m_last_variable_name = expression->m_name.original;
            m_last_pointer = nullptr;
            //check if class type for calling constructors
            object class_type =  m_environment->getValue(expression->m_name.original + "_class");
            if(!std::get_if<null_object>(&class_type)){
                return class_type;
            }

            //else just return variable, probably a function
            return desired_variable;
        }
    }

    object visitAssignExpression(Assign *expression) {
        //get assign value
        llvm::Value *value;
        if(!expression->m_destroy){
            value = std::get<llvm::Value *>(eval(expression->m_value));
        }
        //get variable
        llvm::Value* in = std::get<llvm::Value*>(expression->m_variable->accept(this));
        llvm::Value *variable = m_last_pointer;
        //if this is marked as a destroy, then destroy
        //Assign doubles as a destroy statement for class members
        if(expression->m_destroy){
            destruct(m_last_pointer);
            return null_object();
        }
        //implicit conversion
        if(in->getType()->isFloatTy()){
            value = toFloat(value);
        }
        m_builder.CreateStore(value, variable);
        return null_object();
    }

    object visitLogicExpression(Logic *expression) {
        llvm::Value *left = std::get<llvm::Value *>(eval(expression->m_left));
        llvm::Value *right = std::get<llvm::Value *>(eval(expression->m_right));
        if (expression->m_operator_.type == OR) {
            return m_builder.CreateLogicalOr(left, right, "or");
        } else {
            return m_builder.CreateLogicalAnd(left, right, "and");
        }
    }

    object visitPointerExpression(Pointer *expression) {
        llvm::Value *variable = std::get<llvm::Value *>(eval(expression->m_variable));
        if(variable->getType()->isPointerTy()){
            m_last_pointer = (llvm::AllocaInst*)variable;
            return (llvm::Value*)m_builder.CreateLoad((variable)->getType()->getContainedType(0),variable);
        }else{
            return (llvm::Value*)m_last_pointer;
        }
    }

    object visitMemoryExpression(Memory *expression){
        llvm::Value *variable = std::get<llvm::Value *>(eval(expression->m_right));
        if(expression->m_type == NEW){
            //get size
            llvm::Constant* AllocSize =   llvm::ConstantExpr::getSizeOf(variable->getType());
            //get pointer size 32 bits
            AllocSize = llvm::ConstantExpr::getTruncOrBitCast(AllocSize, llvm::Type::getInt32Ty(m_context));
            //temporary instruction to trick the evil llvm into  thinking its putting its stuff before
            llvm::Instruction* to_move =  m_builder.CreateAlloca(variable->getType(), nullptr);
            llvm::Instruction* to_return = llvm::CallInst::CreateMalloc(m_builder.GetInsertPoint()->getPrevNonDebugInstruction(),llvm::Type::getInt32Ty(m_context),variable->getType(), AllocSize,
                                                                        nullptr, nullptr);
            to_move->eraseFromParent();
            m_builder.CreateStore(variable,(llvm::Value*)to_return);
            return (llvm::Value*)to_return;
        }
        //must be a delete
        //call destructor
        destruct((llvm::AllocaInst*)variable);


        llvm::Instruction* free = llvm::CallInst::CreateFree(variable,m_builder.GetInsertPoint()->getPrevNonDebugInstruction());
        //move to correct spot, curse you llvm
        free->moveAfter(m_builder.GetInsertPoint()->getPrevNonDebugInstruction());

        return null_object() ;
    }

    object visitBracketsExpression(Brackets *expression) {
        //get values
        llvm::Value *right = std::get<llvm::Value *>(eval(expression->m_right));
        llvm::Value *left = std::get<llvm::Value *>(eval(expression->m_left));
        if(left->getType()->isPointerTy()){
            //get member pointer
            llvm::Value *member_ptr = m_builder.CreateGEP(left->getType()->getContainedType(0), left, {right},"arr");
            //set variables for next get expression
            m_last_pointer = (llvm::AllocaInst *) member_ptr;
            //return value
            return (llvm::Value *) m_builder.CreateLoad(((llvm::StructType *) left->getType()->getContainedType(0)),member_ptr, "arr_loaded");
        }

        if(left->getType()->isStructTy()) {
            //check for overload
            llvm::Function *callee_function = m_module.getFunction(
                    left->getType()->getStructName().str() + "_class_brackets_" + getTypeName(right->getType()));
            if (callee_function) {

                return (llvm::Value *) m_builder.CreateCall(callee_function,{right, m_last_pointer /* this needs to be pointer */},"brackets_overload");
            }
        }
        m_error_handler->error(expression->m_line, "Not a brackets type");
        return 0;
    };
    object visitBinaryExpression(Binary *expression) {
        //get values
        llvm::Value *right = std::get<llvm::Value *>(eval(expression->m_right));
        llvm::Value *left = std::get<llvm::Value *>(eval(expression->m_left));
        switch (expression->m_operator_.type) {
            case BANG_EQUAL:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFCmpUNE(left, toFloat(right), "not_equal_floats");
                }else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_bangEqual_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "bangEqual_overload");}
                }  else {
                    return m_builder.CreateICmpNE(left, right, "not_equal_ints");
                }
            case EQUAL_EQUAL:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFCmpUEQ(left, toFloat(right), "equal_floats");
                } else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_equalEqual_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "equalEqual_overload");}
                } else {
                    return m_builder.CreateICmpEQ(left, right, "equal_ints");
                }
            case GREATER:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFCmpUGT(left, toFloat(right), "greater_floats");
                } else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_greater_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "greater_overload");}
                } else {
                    return m_builder.CreateICmpUGT(left, right, "greater_ints");
                }
            case GREATER_EQUAL:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFCmpUGE(left, toFloat(right), "greater_equal_floats");
                } else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_greaterEqual_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "greaterEqual_overload");}
                } else {
                    return m_builder.CreateICmpUGE(left, right, "greater_equal_ints");
                }
            case LESS:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFCmpULT(left, toFloat(right), "less_floats");
                }else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_less_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "less_overload");}
                }  else {
                    return m_builder.CreateICmpULT(left, right, "less_ints");
                }
            case LESS_EQUAL:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFCmpULE(left, toFloat(right), "less_equal_floats");
                }else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_lessEqual_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "lessEqual_overload");}
                }  else {
                    return m_builder.CreateICmpULE(left, right, "less_equal_ints");
                }
            case MINUS:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFSub(left, toFloat(right), "subtract_float");
                }else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_minus_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "minus_overload");}
                }  else {
                    return m_builder.CreateSub(left, right, "subtract_int");
                }
            case SLASH:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFDiv(left, toFloat(right), "divide_float");
                }else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_slash_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "slash_overload");}
                }  else {
                    return m_builder.CreateUDiv(left, right, "divide_int");
                }
            case STAR:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFMul(left, toFloat(right), "multiply_float");
                }else if(left->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_star_" +getTypeName(right->getType()));
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer}, "star_overload");}
                } else {
                    return m_builder.CreateMul(left, right, "multiply_int");
                }
            case PLUS:
                if (left->getType()->isFloatTy()) {
                    return m_builder.CreateFAdd(left, toFloat(right), "add_float");
                } else  if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
                    return m_builder.CreateAdd(left, right, "add_int");
                }else if(left->getType()->isStructTy()){
                    //check for overload
                    llvm::Function *callee_function = m_module.getFunction(left->getType()->getStructName().str() + "_class_plus_" +getTypeName(right->getType()) );
                    if(callee_function){
                        //overload found
                        //this + right
                        return (llvm::Value *) m_builder.CreateCall(callee_function, {right,m_last_pointer /* this needs to be pointer */}, "plus_overload");
                    }
                }
        }
        m_error_handler->error(expression->m_line, "Not a binary type for: " + expression->m_operator_.original);
        return 0;
    };

    object visitUnaryExpression(Unary *expression) {
        //get right value
        llvm::Value *right = std::get<llvm::Value*>(eval(expression->m_right));
        switch (expression->m_operator_.type) {
            case BANG:
                return m_builder.CreateNot(right, "not"); // conditional ! operator
            case MINUS:
                //make number negative
                if (right->getType()->isFloatTy()) {
                    return m_builder.CreateFNeg(right, "negative_float");
                } else if(right->getType()->isStructTy()){
                    llvm::Function *callee_function = m_module.getFunction(right->getType()->getStructName().str() + "_class_negative");
                    if(callee_function){return (llvm::Value *) m_builder.CreateCall(callee_function, {m_last_pointer}, "negative_overload");}
                } else {
                    return m_builder.CreateNeg(right, "negative_int");
                }
        }
        m_error_handler->error(expression->m_line, "Not a unary type for: " + expression->m_operator_.original);
        return null_object();
    };

    object visitGroupingExpression(Grouping *expression) {
        return eval(expression->m_expression);
    };

    object visitLiteralExpression(Literal *expression) {

        if (int *int_number = std::get_if<int>(&expression->m_value)) { //create integer. All ints are 32 bit.
            return llvm::ConstantInt::get(m_context, llvm::APInt(32, *int_number));
        }
        else if (float *number = std::get_if<float>(&expression->m_value)) { //create float
            return llvm::ConstantFP::get(m_context, llvm::APFloat(*number));
        }
        else if (bool *boolean = std::get_if<bool>(&expression->m_value)) {        //create bool. LLVM just treats them as ints.
            return llvm::ConstantInt::get(m_context, llvm::APInt(1,  *boolean));
        }
        else if (std::string *s = std::get_if<std::string>(&expression->m_value)) { //create constant string
            return (llvm::Value *) m_builder.CreateGlobalStringPtr(*s);
        }
        //literal cannot be created
        m_error_handler->error(expression->m_line,"Unsupported literal.");
        return null_object();
    };

private:
    //set up llvm
    llvm::LLVMContext m_context;
    llvm::IRBuilder<> m_builder{m_context};
    llvm::Module m_module{"Doge Language", m_context};
    //additional files
    std::map<std::string,statementList> m_external_files;
    //additional cpp or .o to link.
    std::vector<std::string> externals;
    //filename of output
    std::string m_object_filename = "output.o";

    //current environment
    Environment *m_environment;
    //top level environment
    Environment *m_top;
    //error handler
    ErrorHandler *m_error_handler;
    //last processed data
    llvm::Value *m_last_class = nullptr;
    std::string m_last_variable_name = "";
    llvm::AllocaInst *m_last_pointer;

    //evaluate an expression
    object eval(Expression *expression) {
        return expression->accept(this);
    }

    //evaluate a block and manage the environments
    void evalBlock(statementList statements, Environment *environment) {
        //swap envs
        Environment *prev_env = m_environment;
        m_environment = environment;
        //run statements
        for (Statement *inner_statement: statements) {
            inner_statement->accept(this);
        }
        //destruct local vars
        if(!m_returned){
            destruct();
        }
        m_returned = false;

        //cleanup
        delete environment;
        //back to original env
        m_environment = prev_env;
    }
    //call destructors for local variables
    void destruct(){
        for (std::pair<std::string, object> element :  m_environment->m_values)
        {
            //check if pointer exists to variable
            if( llvm::AllocaInst** val = std::get_if< llvm::AllocaInst* >(&element.second)){
                llvm::AllocaInst* variable = *val;
                destruct(variable);
            }

        }
    }
    //destruct an object
    void destruct(llvm::AllocaInst* variable){
        llvm::Type* type = variable->getType()->getNonOpaquePointerElementType();
        //check if class
        if(type->isStructTy()) {

            //call default constructor
            llvm::Function *default_destructor = m_module.getFunction(
                    type->getStructName().str() + "_class_" + "destructor_default");
            if (default_destructor) {
                m_builder.CreateCall(default_destructor, {variable});
            }
            //call specified destructor
            llvm::Function *destructor = m_module.getFunction(
                    type->getStructName().str() + "_class_" + "destruct");
            if (destructor) {
                m_builder.CreateCall(destructor, {variable});
            }

        }


    }

    //create alloca at entry block. This is for easier analysis and optimization of code by llvm.
    llvm::AllocaInst *blockAllocation(llvm::Function *function, const std::string &variable_name, llvm::Type *type) {
        llvm::IRBuilder<> block(&function->getEntryBlock(),function->getEntryBlock().begin());
        return block.CreateAlloca(type, 0,variable_name);
    }
    //get indices array for accessing class members
    std::vector<llvm::Value *> getIndices(const int index) {
        llvm::Value *member_index = llvm::ConstantInt::get(m_context, llvm::APInt(32, index, true));
        std::vector<llvm::Value *> indices(2);
        indices[0] = llvm::ConstantInt::get(m_context, llvm::APInt(32, 0, true));
        indices[1] = member_index;
        return indices;
    }

    //get type from token
    llvm::Type *getType(const Token type) {
        std::string type_name = type.original;
        bool ptr = false;
        if(type_name.find("_ptr") != std::string::npos) {
            ptr = true;
            type_name.erase(type_name.length()-4);
        }

        //primitives
        if (type_name == "bool") {
            if(ptr){return  llvm::Type::getInt1PtrTy(m_context);}
            return llvm::Type::getInt1Ty(m_context);
        } else if (type_name == "int") {
            if(ptr){return  llvm::Type::getInt32PtrTy(m_context);}
            return llvm::Type::getInt32Ty(m_context);
        } else if (type_name == "float") {
            if(ptr){return  llvm::Type::getFloatPtrTy(m_context);}
            return llvm::Type::getFloatTy(m_context);
        } else if (type_name == "chars") {
            if(ptr){return  llvm::PointerType::get(llvm::Type::getInt8PtrTy(m_context),0);}
            return llvm::Type::getInt8PtrTy(m_context);
        }
        else if (type_name == "void") {
            return llvm::Type::getVoidTy(m_context);
        }
        //might be class
        object class_type = m_environment->getValue(type_name + "_class");
        if (llvm::Type **class_def = std::get_if<llvm::Type *>(&class_type)) {
            if(ptr){return  llvm::PointerType::get(*class_def,0);}
            return *class_def;
        }
        //type not supported, just return float.
        m_error_handler->error("Unsupported type: " + type_name);
        return llvm::Type::getFloatTy(m_context);
    }
    //get name from type
    std::string getTypeName(llvm::Type* type) {
        //primitives
        if(type->isFloatTy()){
            return "float";
        }else if(type->isIntegerTy(32)){
            return "int";
        }else if(type->isIntegerTy(1)){
            return "bool";
        }else if(type == llvm::Type::getInt8PtrTy(m_context)){
            return "chars";
        }else if(type->isVoidTy()){
            return "void";
        }else if(type->isStructTy()){
            return type->getStructName().str();
        }else if(type == llvm::Type::getFloatPtrTy(m_context)){
            return "float_ptr";
        }else if(type == llvm::Type::getInt32PtrTy(m_context)){
            return "int_ptr";
        }else if(type == llvm::Type::getInt1PtrTy(m_context)){
            return "bool_ptr";
        }else if(type->isPointerTy()){
            return type->getContainedType(0)->getStructName().str() + "_ptr";
        }
        return "null";
    }

    llvm::Value* toFloat(llvm::Value* not_float){
        if(not_float->getType()->isFloatTy()){
            //is already float
            return not_float;
        }
        else if(not_float->getType()->isIntegerTy()){
            return m_builder.CreateSIToFP(not_float, llvm::Type::getFloatTy(m_context), "float_cast");
        }
        m_error_handler->error("Unsupported conversion: " + not_float->getName().str());
        return not_float;
    }

};
#endif //PROGRAM_IRCOMPILER_HPP
