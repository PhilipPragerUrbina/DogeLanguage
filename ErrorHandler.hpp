//
// Created by Philip on 5/20/2022.
//

#ifndef PROGRAM_ERRORHANDLER_HPP
#define PROGRAM_ERRORHANDLER_HPP

#include <string>
#include <iostream>

#include "Color.hpp"

class ErrorHandler {
public:
    std::string m_name;
    std::string m_file;
    ErrorHandler(std::string name = "Program", std::string file = ""){
        m_name = name;
        m_file = file;
    }
    //throw normal error
    void error(int line, std::string message = "unknown"){
        Color::start(RED);
        std::cout << m_file+ " Line " << line+1 <<  " " << m_name << " Error: " << message << "\n";
        Color::end();
        m_error_number++;
    }
    void error(std::string message = "unknown"){
        Color::start(RED);
        std::cout <<  m_name << " "<< m_file+ " Error: " << message << "\n";
        Color::end();
        m_error_number++;
    }
    //throw warning
    void warning(int line, std::string message){
        Color::start(YELLOW);
        std::cout << m_file+" Line " << line+1 << " "<< m_name <<" Warning: " << message << "\n";
        Color::end();
        m_warning_number++;
    }
    void warning(std::string message){
        Color::start(YELLOW);
        std::cout <<  m_name<< " " <<m_file+" Warning: " << message << "\n";
        Color::end();
        m_warning_number++;
    }
    //throw error and exit
    void fatalError(int line, std::string message = "unknown"){
        std::cerr << m_file+" Line " << line+1 << "Fatal Error: " << message << "\n";
        exit(1);
    }
    void fatalError(std::string message = "unknown"){
        std::cerr << m_file+" Fatal Error: " << message << "\n";
        exit(1);
    }
    //get number of errors
    int getErrorNumber(){
        return m_error_number;
    }
    int getWarningNumber(){
        return m_warning_number;
    }
    bool hasErrors(){
        return m_error_number > 0;
    }
    bool hasWarnings(){
        return m_warning_number > 0;
    }
private:
    int m_error_number = 0;
    int m_warning_number = 0;
};

#endif PROGRAM_ERRORHANDLER_HPP