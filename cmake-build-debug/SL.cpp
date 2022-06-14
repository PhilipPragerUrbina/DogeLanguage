#include <iostream>
#include<string>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

// standard library externals

//get char string input
extern "C" DLLEXPORT const char* charsIn() {
    std::string in;
    std::cin >> in;
    std::string* out =   new std::string(in);
    return out->data();
}

//output
extern "C" DLLEXPORT void printChars_chars(char* chars) {
    std::cout <<chars;
}


//conversions
extern "C" DLLEXPORT const char* toChars_float(float a) {
    std::string* out =   new std::string(std::to_string(a));
    return out->data();
}
extern "C" DLLEXPORT const char* toChars_int(int a) {
    std::string* out =   new std::string(std::to_string(a));
    return out->data();
}
extern "C" DLLEXPORT float toFloat_int(int a) {
    return (float)a;
}
extern "C" DLLEXPORT float toFloat_chars(const char* a) {
    std::string in = std::string (a);
    return std::stof(in);
}
extern "C" DLLEXPORT int toInt_float(float a) {
    return (int)a;
}

//utilities

//add two chars
extern "C" DLLEXPORT const char* concat_chars_chars(char* a,char* b) {
    std::string* out = new std::string((std::string(a)+std::string(b)));
    return out->data();
}
//power of
extern "C" DLLEXPORT float power_float_float(float a, float b) {
    return pow(a,b);
}
