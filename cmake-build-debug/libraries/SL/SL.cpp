#include <iostream>
#include<string>

// standard library externals
//get char string input
extern "C"  const char* charsIn() {
    std::string in;
    std::cin >> in;
    std::string* out =   new std::string(in);
    return out->data();
}

//output
extern "C"  void printChars_chars(char* chars) {
    std::cout <<chars;
}


//conversions
extern "C"  const char* toChars_float(float a) {
    std::string* out =   new std::string(std::to_string(a));
    return out->data();
}
extern "C"  const char* toChars_int(int a) {
    std::string* out =   new std::string(std::to_string(a));
    return out->data();
}
extern "C"  float toFloat_chars(const char* a) {
    std::string in = std::string (a);
    return std::stof(in);
}
extern "C"  int toInt_float(float a) {
    return (int)a;
}

//utilities

//add two chars
extern "C"  const char* concat_chars_chars(char* a,char* b) {
    std::string* out = new std::string((std::string(a)+std::string(b)));
    return out->data();
}
//power of
extern "C"  float power_float_float(float a, float b) {
    return std::pow(a,b);
}

