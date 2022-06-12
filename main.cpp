//define language version
#define DOGE_LANGUAGE_VERSION "v0.5"

#include <iostream>

#include "File.hpp"
#include "Scanner.hpp"
#include "Parser.hpp"
#include "IRCompiler.hpp"
#include "Analyzer.hpp"

#include <windows.h>
#include <filesystem>


int main(int argc, char* args[]) {
    //enable colors
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
    //show starting message
    Color::start(BLUE);
    std::cout << "Using DogeLang " << DOGE_LANGUAGE_VERSION << " \n";
    Color::end();

    //default files
    std::string source_filename = "main.doge";
    std::string object_filename = "output.o";
    std::string executable_filename = "output.exe";

    //check if other file specified
    if (argc > 1){ source_filename = args[1];}

    //TODO work with multiple custom source files
    //check if source was modified since last build
    std::filesystem::path source_path(source_filename);
    auto source_modify_time = last_write_time(source_path);
    std::filesystem::path build_path(executable_filename);
    if(exists(build_path)){
        auto build_modify_time = last_write_time(build_path);
        if(source_modify_time < build_modify_time){
            //source has not changed, just run
            std::cout << "\n No changes detected. Running build... \n";
            system(executable_filename.c_str());
            return 0;
        }
    }

    //create error handler
    ErrorHandler error_handler;


    //read file
    File main_file(source_filename);
    //file did not open
    if(!main_file.read()){return 1;}
    //scan file
    Scanner scanner(main_file.getData(),&error_handler);
    scanner.scan();
    //check for errors
    if(error_handler.hasErrors()){return 1;}
    //get tokens
    std::vector<Token> tokens = scanner.getTokens();
    //parse code
    Parser parser(tokens,&error_handler);
    statementList statements  = parser.parse();
    if(error_handler.hasErrors()){return 1;}

    std::map<std::string,statementList> external_files;

    for(std::string include : parser.m_includes){
        File file(include);
        if(!file.read()){return 1;}
        Scanner include_scanner(file.getData(),&error_handler);
        include_scanner.scan();
        Parser include_parser(include_scanner.getTokens(),&error_handler);
        external_files[include] = include_parser.parse();
        if(error_handler.hasErrors()){return 1;}
    }
    for(std::string import : parser.m_imports){
        File file(import + ".dogel");
        if(!file.read()){return 1;}
        Scanner include_scanner(file.getData(),&error_handler);
        include_scanner.scan();
        Parser include_parser(include_scanner.getTokens(),&error_handler);
        external_files[import] = include_parser.parse();
        if(error_handler.hasErrors()){return 1;}
    }


    //check
    Analyzer analyzer;
    if(analyzer.check(statements,external_files,&error_handler)){
        return 1;
    }

    //compile
    std::cout << "\n Compiling... \n" ;
    IRCompiler compiler;
    compiler.compile(statements,external_files,&error_handler);

    //check for errors
    if(error_handler.hasErrors()){
        return 1;
    }

    //build ir
    std::cout << "\n Compile build: \n \n" ;
    compiler.print();

    //optimize ir
    std::cout << "\n Optimizing... \n" ;
    compiler.optimize();
    std::cout << "\n optimize build: \n \n" ;
    compiler.print();

    //build to .o
    compiler.build(object_filename);

    //assemble
    std::cout << "\n Assembling... \n";
    compiler.assemble(executable_filename);

    //run file
    std::cout << "\n Running... \n";
    system(executable_filename.c_str());

    return 0;
}


/*
//choose random colors for syntax highlighting
    std::map<int,TextColor> colors;
    //same colors very time
    srand(0);
    for ( int i = LEFT_PAREN; i != END; i++ )
    {
        TextColor random = static_cast<TextColor>(rand() % YELLOW);
         colors[i] = random;
    }

      //display tokens
    std::cout << "\n \n Detected Tokens: \n \n";
    for(Token token:tokens){
        Color::start(colors[token.type]);
        std::cout << token.original << "  Line: " << token.line << " Type: " << token.type << "\n";
        Color::end();
    }
    std::cout << "\n \n Detected statementList: \n \n";
    //display code
    int last_line = 0;
    for(Token token:tokens){
        if(last_line != token.line){
            last_line = token.line;
            std::cout << "\n";
        }
        Color::start(colors[token.type]);
        std::cout << token.original;
        Color::end();
    }
     */

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
