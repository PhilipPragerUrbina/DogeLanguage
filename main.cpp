//define language version
#define DOGE_LANGUAGE_VERSION "v1.0"



#include "popl.hpp"
#include <iostream>


#include "Scanner.hpp"
#include "Parser.hpp"
#include "IRCompiler.hpp"
#include "Analyzer.hpp"
#include "File.hpp"
#include <windows.h>
#include <filesystem>
bool getIncludes(Parser &parser, std::filesystem::file_time_type &source_modify_time, ErrorHandler &error_handler,
                 std::map<std::string, statementList> &external_files, std::map<std::string ,std::vector<Token >>* templates ,
std::vector<ClassStatement*>* templates_to_resolve);

int main(int argc, char **argv) {

    //create command line interface
    popl::OptionParser command_line((std::string("Doge compiler ") + DOGE_LANGUAGE_VERSION));
    //set options
    auto help_option = command_line.add<popl::Switch>("h", "help", "Print help message");
    auto source_filename_option = command_line.add<popl::Value<std::string>>("s", "source", "Set source filename. Can be .doge or .dogel","main.doge");
    auto output_filename_option = command_line.add<popl::Value<std::string>>("o", "output", "Set output filename. Can be .o, .exe, .s, or .ll","output.exe");
    auto vcvarsall_option = command_line.add<popl::Value<std::string>>("m", "vcvarsall", "Set the folder where the visual studio vcvarsall.bat is located for msvc. Probably needs to be in quotations","");
    auto print_llvm_option = command_line.add<popl::Switch>("p", "print", "Print llvm ir");
    auto error_option = command_line.add<popl::Switch>("e", "error", "Only error check");
    auto optimize_disable_option = command_line.add<popl::Switch>("u", "unoptimized", "Disable optimizations");
    auto run_disable_option = command_line.add<popl::Switch>("r", "disrun", "Dont run executable on build. For if you use another build tool or IDE that will run it for you already");
    //parse
    command_line.parse(argc, argv);
    //show help
    if (help_option->is_set()) { std::cout << command_line << "\n"; return 0;}

    //get filenames
    std::string source_filename = source_filename_option->value();
    std::string output_filename = output_filename_option->value();
    std::string vcvarsall_location = vcvarsall_option->value();
    std::string object_filename = "";
    std::string code_filename = "";
    std::string executable_filename = "";
    std::filesystem::path build_path(output_filename);
    //set up files that need to be generated
    if (build_path.extension() == ".exe") {
        executable_filename = output_filename;
       auto  object_path = build_path;
        object_path.replace_extension(".o");
        object_filename =object_path.string();
    } else if (build_path.extension() == ".s" || build_path.extension() == ".o") {
        object_filename = output_filename;
    } else if (build_path.extension() == ".ll") {
        code_filename = output_filename;
    } else {
        std::cerr << "Output must be .o,.ll,.s, or .exe file: " + output_filename;
        return 1;
    }

    //enable colors
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
    //show starting message
    Color::start(BLUE);
    std::cout << "Using Doge Language " << DOGE_LANGUAGE_VERSION << " \n";
    Color::end();
    Color::start(YELLOW);
    std::cout << "Building " << source_filename << " to " << object_filename << " "<< code_filename<< " " << executable_filename << " \n";
    Color::end();

    //check if source is valid
    std::filesystem::path source_path(source_filename);
    if (!exists(source_path)) {
        std::cerr << "Cannot find source file: " + source_filename;
        return 1;
    }
    if (source_path.extension() != ".doge" && source_path.extension() != ".dogel") {
        std::cerr << "File is not a .doge or .dogel file: " + source_filename;
        return 1;
    }
    auto source_modify_time = last_write_time(source_path);
//templates
    std::map<std::string ,std::vector<Token >> templates ;
    std::vector<ClassStatement*> templates_to_resolve;

    //create error handler
    ErrorHandler error_handler;
    std::cout << "\nParsing: " + source_filename +" \n";
    //read file
    File main_file(source_filename);
    //file did not open
    if (!main_file.read()) { return 1; }
    //scan file
    Scanner scanner(main_file.getData(), &error_handler,source_filename);
    scanner.scan();
    //check for errors
    if (error_handler.hasErrors()) { return 1; }
    //get tokens
    std::vector<Token> tokens = scanner.getTokens();
    //parse code
    Parser parser(tokens, &error_handler,source_filename,    &templates,&templates_to_resolve);
    statementList statements = parser.parse();
    if (error_handler.hasErrors()) { return 1; }
    std::cout << "\nFinding... \n";
    std::map<std::string, statementList> external_files;
    if(!getIncludes(parser, source_modify_time, error_handler, external_files,&templates,&templates_to_resolve)){
        return 1;
    };


    //check if source was modified
        if (exists(build_path)) {
            auto build_modify_time = last_write_time(build_path);
            if (source_modify_time < build_modify_time) {
                //source has not changed since build, just run or exit
                Color::start(GREEN);
                std::cout << "\nNo changes detected.\n";
                Color::end();
                if(executable_filename != "" && !run_disable_option->is_set()){
                    std::cout << "\nRunning build...\n";
                    system(executable_filename.c_str());
                }
                return 0;
            }
        }
        //parse templates that are in other files
        Parser(&templates,templates_to_resolve);


    //error check
    std::cout << "\nChecking... \n";
    Analyzer analyzer;
    if (analyzer.check(statements, external_files, &error_handler,source_filename)) {return 1;}
    //check if set to fully compile
    if(error_option->is_set()){
        Color::start(GREEN);
        std::cout << "\n No errors found.";
        Color::end();
        return 0;
    }
    //compile
    std::cout << "\nCompiling... \n";
    IRCompiler compiler;
    compiler.compile(statements, external_files, &error_handler,source_filename);
    //check for errors
    if (error_handler.hasErrors()) {return 1;}
    //show warnings
    if(error_handler.hasWarnings()){
        Color::start(CYAN);
        std::cout << "\n" << error_handler.getWarningNumber() << " warnings! Continue anyway? y/n \n";
        Color::end();
        std::string in;
        std::cin >> in;
        if(in != "y"){
            return 1;
        }

    }
    //optimize ir
    if(!optimize_disable_option->is_set()){
        std::cout << "\nOptimizing... \n";
        compiler.optimize();
    }
    //Print ir
    if(print_llvm_option->is_set()){
        compiler.print();
    }
    //output llvm ir to file
    if(code_filename != ""){
        std::cout << "\nOutputting llvm ir... \n";
        compiler.printFile(code_filename);
    }
    //build to .o or .s
    if(object_filename != ""){
        std::cout << "\nBuilding to object file... \n";
        compiler.build(object_filename);
    }
    //assemble
    if(executable_filename != ""){
        std::cout << "\nAssembling... \n";
        if(compiler.assemble(executable_filename, vcvarsall_location) && !run_disable_option->is_set()){
            //run file
            std::cout << "\nRunning... \n";
            system(executable_filename.c_str());
        };
    }
    Color::start(CYAN);
    std::cout << "\nFinished.\n";
    Color::end();
    return 0;
}

bool getIncludes(Parser &parser, std::filesystem::file_time_type &source_modify_time, ErrorHandler &error_handler,
                 std::map<std::string, statementList> &external_files, std::map<std::string ,std::vector<Token >>* templates ,
std::vector<ClassStatement*>* templates_to_resolve) {//get included files
    for (std::string include: parser.m_includes) {
        std::cout << "\nParsing: " + include +" \n";
        File file(include);
        if (!file.read()) {  return false;  }
        //check for changes
        auto include_modify_time = last_write_time(std::filesystem::path(include));
        if (source_modify_time < include_modify_time) {
            source_modify_time = include_modify_time;
        }
        Scanner include_scanner(file.getData(), &error_handler,include);
        include_scanner.scan();
        Parser include_parser(include_scanner.getTokens(), &error_handler,include,templates,templates_to_resolve);
        external_files[include] = include_parser.parse();
        if (error_handler.hasErrors()) {  return false; }
       if(!getIncludes(include_parser,source_modify_time,error_handler,external_files,templates,templates_to_resolve)){
           return false;
       }
    }
    for (std::string import: parser.m_imports) {
        std::cout << "\nParsing: " + import  +".dogel \n";
        File* file = new File(import + ".dogel");
        std::string link_directory = "";
        if (!file->read()) {
            delete file;
            //check in exe directory
            File* new_file = new File(getExeDirectory("libraries/" + import+".dogel"));
            if(!new_file->read()){
                return false;
            }
            link_directory = getExeDirectory("libraries/");
            file = new_file;
        }
        //check for changes
        //check for changes
        auto include_modify_time = last_write_time(std::filesystem::path(  file->getName()));
        if (source_modify_time < include_modify_time) {
            source_modify_time = include_modify_time;
        }
        Scanner include_scanner(file->getData(), &error_handler,import);
        include_scanner.scan();
        Parser include_parser(include_scanner.getTokens(), &error_handler,import,templates,templates_to_resolve);
        include_parser.link_directory = link_directory;
        external_files[import] = include_parser.parse();
        if (error_handler.hasErrors()) {   return false; }
        delete file;
        if(!getIncludes(include_parser,source_modify_time,error_handler,external_files,templates,templates_to_resolve)){
            return false;
        }
    }
    return true;
}
/*
//generate graph vis  file of code
DotFileGenerator graph;
graph.print(statements);
*/
/*
    //interpret
    Interpreter runtime;

    //output
    std::cout << "\n build: \n \n" ;

    std::string out = runtime.run(statements,&error_handler);

      //check for errors
    if(error_handler.hasErrors()){
        return 1;
    }

      Color::start(GREEN);
    std::cout<<"\n \n Exited with: " << out << " \n";
    Color::end();
*/
