//
// Created by Philip on 5/20/2022.
//

#ifndef PROGRAM_COLOR_HPP
#define PROGRAM_COLOR_HPP

#include <string>
#include <iostream>

enum TextColor{
    BLUE,GREEN, CYAN, RED, MAGENTA,YELLOW
};
class Color {
public:
    //start a color. Then you can output to the console in that color.
    static void start(TextColor color){

        if(*getPairNumber() > 0){std::cerr << "Color already active \n"; return;}
        *getPairNumber() += 1;

        std::string col = "\033[0m";
        if (color == BLUE) col = "\033[0;34m";
        else if (color == GREEN) col = "\033[0;32m";
        else if (color == CYAN) col = "\033[0;36m";
        else if (color == RED) col = "\033[0;31m";
        else if (color == MAGENTA) col = "\033[0;35m";
        else if (color == YELLOW) col = "\033[0;33m";
        std::cout << col;
    }
    //stop using the color
    static void end(){
        *getPairNumber() -= 1;
        std::cout << "\033[0m";
    }
private:
    //number of paired starts and ends
    static int* getPairNumber() {
        static int num = 0;
        return &num;
    }
};

#endif PROGRAM_COLOR_HPP