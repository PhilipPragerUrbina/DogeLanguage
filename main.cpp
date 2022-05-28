#define DOGE_LANGUAGE_VERSION "v0.2"

#include <iostream>

#include "File.hpp"
#include "ErrorHandler.hpp"
#include "Scanner.hpp"
#include "Parser.hpp"
#include "Printers.hpp"
#include "Interpreter.hpp"

#include <windows.h>
int main(int argc, char* args[]) {
    //enable colors
    system(("chcp " + std::to_string(CP_UTF8)).c_str());

    //show starting message
    Color::start(BLUE);
    std::cout << "Using DogeLang " << DOGE_LANGUAGE_VERSION << " \n";
    Color::end();

    //default file
    std::string filename = "main.doge";
    //check if other file specified
    if (argc > 1){filename = args[1];}

    //read file
    File main_file(filename);
    //file did not open
    if(!main_file.read()){
        return 1;
    }

    //create error handler
    ErrorHandler error_handler;

    //scan file
    Scanner scanner(main_file.getData(),&error_handler);
    scanner.scan();

    //check for errors
    if(error_handler.hasErrors()){
        return 1;
    }
    //get tokens
    std::vector<Token> tokens = scanner.getTokens();

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

    //parse code
    Parser parser(tokens,&error_handler);
   statementList statements  = parser.parse();

    //check for errors
    if(error_handler.hasErrors()){
        return 1;
    }

    //generate graph vis  file of code
    DotFileGenerator graph;
    graph.print(statements);

    //interpret
    Interpreter runtime;

    //output
    std::cout << "\n output: \n \n" ;

    std::string out = runtime.run(statements,&error_handler);

    //check for errors
    if(error_handler.hasErrors()){
        return 1;
    }



    Color::start(GREEN);
    std::cout<<"\n \n Exited with: " << out << " \n";
    Color::end();

    return 0;
}
