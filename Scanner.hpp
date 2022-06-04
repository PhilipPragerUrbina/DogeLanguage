//
// Created by Philip on 5/20/2022.
//
#ifndef PROGRAM_SCANNER_HPP
#define PROGRAM_SCANNER_HPP
#include <iostream>
#include <vector>
#include <unordered_map>
#include <variant>
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include "ErrorHandler.hpp"

//types
//object that represents null
struct null_object{};

//forward declarations
class Callable;
class Interpreter;
class Environment;
//Pointer type
struct Reference{
    Reference(std::string name, Environment* env){
        m_name = name   ;
        m_env = env;

    }
    Environment* m_env = nullptr;
    std::string m_name;
};

struct Class {
public:
    Class(std::string name = "",Environment* environment = nullptr){
        m_name = name;
        m_environment = environment;

    }
    std::string m_name;
    Environment* m_environment;
};
struct ClassObject {
public:
    ClassObject(Class class_, Environment* environment = nullptr){
        m_class = class_;
        m_environment = environment;
    }
    Class m_class;
    Environment* m_environment;
};
struct Signal{
    Signal(int signal){
        m_signal = signal;
    }
    int m_signal;
};

//generic object with all supported types
typedef std::variant<int, float, std::string, bool, null_object, Callable,Reference,Class, ClassObject, Signal, llvm::Value*, llvm::Function*, llvm::AllocaInst*> object;

//function pointer
typedef object (*callPointer)( Interpreter* ,std::vector<object>);
class FunctionStatement;

//function type
struct Callable {
public:
    Callable(int args, callPointer func,  FunctionStatement* declaration = nullptr){
        m_argument_number = args; m_call = func;
        m_declaration = declaration;
    }
    FunctionStatement* m_declaration;
    int m_argument_number;
    callPointer m_call;
};




enum TokenType {
    //single tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
    //two char tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    //literals
    IDENTIFIER, STRING, INTEGER, FLOATING,
    //words.
    AND, CLASS, ELSE, FALSE, FOR, IF, NIL, OR,
    RETURN, SUPER, THIS, TRUE, VAR, WHILE,REFERENCE,CONST,BREAK, CONTINUE,HASH,IMPORT, INCLUDE,INC, DEC,
    //end of file
    END
};


struct Token{
public:
    //object value
    object value;
    //original text
    std::string original;
    //type
    TokenType type;
    //line in file
    int line = 0;
};





class Scanner {
public:
    //take text input
    Scanner(std::vector<std::string> lines, ErrorHandler* error_handler){
        m_lines = lines;
        m_error_handler = error_handler;

        //set keywords
        m_keywords[ "and" ] = AND;
        m_keywords[ "class" ] = CLASS;
        m_keywords[ "else" ] = ELSE;
        m_keywords[ "false" ] = FALSE;
        m_keywords[ "for" ] = FOR;
        m_keywords[ "function" ] = VAR;
        m_keywords[ "if" ] = IF;
        m_keywords[ "null_object" ] = NIL;
        m_keywords[ "or" ] = OR;
        m_keywords[ "return" ] = RETURN;
        m_keywords[ "break" ] = BREAK;
        m_keywords[ "continue" ] = CONTINUE;
        m_keywords[ "super" ] = SUPER;
        m_keywords[ "const" ] = CONST;
        m_keywords[ "true" ] = TRUE;
        m_keywords[ "var" ] = VAR;
        //alternative vars
        m_keywords[ "bool" ] = VAR;
        m_keywords[ "ptr" ] = VAR;
        m_keywords[ "pointer" ] = VAR;
        m_keywords[ "int" ] = VAR;
        m_keywords[ "float" ] = VAR;
        m_keywords[ "string" ] = VAR;
        m_keywords[ "while" ] = WHILE;
        m_keywords[ "include" ] = INCLUDE;
        m_keywords[ "import" ] = IMPORT;
    }

    //scan text to create tokens
    void scan(){
        //scanner should be used once per text
        if( m_position != 0){std::cerr << "Scanner has already scanned \n";}

        //states that work across different lines
        bool string  = false;
        bool multi_comment = false;
        //for each line in file
        for(std::string line: m_lines){
            //current char in line
           m_position = 0;
           //states
           bool comment = false;
            bool number = false;
            bool floating = false;
            bool identifier = false;

            //for each char
            for (const char &character : line) {
                //current char
                m_current_character = character;

                //skip characters completely
                if(comment || m_skip || multi_comment){
                    //skip singe character
                    if(m_skip){m_skip = false;}
                    //close comment
                    if(character == '*'){
                        if (match('/')) {multi_comment = false;}
                    }
                    m_position++; continue;
                }
                if(string){
                    //close string
                    if(character == '"'){
                        string = false;
                        addToken(STRING, m_word, "\"" + m_word + "\"");
                    }else { //add to string
                        m_word.push_back(character);
                    }
                    m_position++; continue;
                }
                if(number){
                    if(isDigit(character) || character == '.') {
                        if(character == '.'){ //check if float
                            if(floating == true){m_error_handler->error(m_line,"Double point in float");}
                            floating = true;
                        }
                        m_word.push_back(character); m_position++;
                        if(m_position  < line.length()){continue;}
                    }
                    //no longer number, add number
                    if(floating){
                        addToken(FLOATING , std::stof(m_word) , m_word );
                    }else{
                        addToken(INTEGER,  std::stoi(m_word), m_word );
                    }
                        number = false; floating = false;
                    if(m_position  == line.length()){continue;} //end of line. Character has already been added.
                }
                if(identifier){
                    if(isAlpha(character) || isDigit(character)) {
                        m_word.push_back(character); m_position++;
                        if(m_position  < line.length()){continue;}
                    }
                    addIdentifier(m_word);
                    identifier = false;
                    if(m_position  >= line.length()){continue;}
                }

                switch (character) {
                    //single character
                    case '(': addToken(LEFT_PAREN); break;
                    case '#': addToken(HASH); break;
                    case ')': addToken(RIGHT_PAREN); break;
                    case '{': addToken(LEFT_BRACE); break;
                    case '}': addToken(RIGHT_BRACE); break;
                    case ',': addToken(COMMA); break;
                    case '.': addToken(DOT); break;
                    case '-': addToken(match('+') ? DEC : MINUS); break;
                    case '+': addToken(match('+') ? INC : PLUS); break;
                    case ';': addToken(SEMICOLON); break;
                    case '*':addToken(STAR);break;
                    case '"': string = true; break;
                    //double characters
                    case '!': addToken(match('=') ? BANG_EQUAL : BANG); break;
                    case '=': addToken(match('=') ? EQUAL_EQUAL : EQUAL); break;
                    case '<': addToken(match('=') ? LESS_EQUAL : LESS); break;
                    case '>': addToken(match('=') ? GREATER_EQUAL : GREATER); break;
                    case '|': if (match('|')) {addToken(OR);} break;
                    case '&': addToken(match('&') ? AND : REFERENCE); break;
                    case '/':
                        if (match('/')) { comment = true; }  //single line comment
                        else if (match('*')) { multi_comment = true; } //multi line comment
                        else { addToken(SLASH); } break; //divide
                    case ' ': break; //skip spaces
                    default:
                        //check if number
                        if(isDigit(character)){
                            number = true;
                            m_word.push_back(character);      //first character of number should be saved
                            if(m_position  == line.length()-1){addToken( INTEGER, std::stoi(m_word), m_word );} //save right away if end of line
                        } else if (isAlpha(character)) {
                          identifier = true;
                          m_word.push_back(character);
                          if(m_position  == line.length()-1){addIdentifier(m_word);}
                        }else{
                            m_error_handler->error(m_line, "unknown character: " + m_current_character);
                        }
                }
                m_position++; //increment char
            }
            m_line++;
        }
        addToken(END, 0 , " "); //end of file
    }
    //get tokens
    std::vector<Token> getTokens(){return m_tokens;}


private:
    //error handler
    ErrorHandler* m_error_handler;
    //keywords
    std::unordered_map<std::string, TokenType> m_keywords;
    //vectors
    std::vector<std::string> m_lines;
    std::vector<Token> m_tokens;
    //positions
    int m_line = 0;
    int m_position = 0;
    bool m_skip = false;
    //buffers
    std::string m_current_character = " ";
    std::string m_word = "";

    //checks
    bool isDigit(const char &character) {
        return character >= '0' && character <= '9';
    }
    bool isAlpha(const char &character) {
        return (character >= 'a' && character <= 'z') ||
               (character >= 'A' && character <= 'Z') ||
                character == '_';
    }
    bool match(const char &next){
        //is next character end of file
        if( m_position+1 >= m_lines[m_line].length() ){return false;}
        bool matched =  m_lines[m_line][m_position+1] == next; //is next character the right one
        if(matched){m_current_character.push_back(next); m_skip = true;} //don't process character if matched
        return matched;
    }
    //add new token that is identifier
    void addIdentifier(std::string word){
        //try to see if matches keywords
        if ( m_keywords.find(word) == m_keywords.end() ) {
            addToken(IDENTIFIER, word, word );
        } else {
            addToken(m_keywords[word], word, word );
        }
    }
    //add a new token
    void addToken(TokenType type, object value = 0, std::string original = ""){
        //set m_values
        Token token;
        token.line = m_line;
        token.value = value;
        token.type = type;
        if(original == ""){token.original = m_current_character;
        }else{token.original = original;}
        m_tokens.push_back(token);
        m_word = ""; //reset buffer
    }
};
#endif PROGRAM_SCANNER_HPP