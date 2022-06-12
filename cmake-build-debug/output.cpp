#include <iostream>
#include<string>
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif


//temporary standard library
//print a char from a float
extern "C" DLLEXPORT int putF_float(float X) {
    fputc((char)X, stdout);
    return 0;
}
//print a constant string
extern "C" DLLEXPORT int printC_chars(char* chars) {
    std::cout << chars;
    return 0;
}
extern "C" DLLEXPORT int newLine() {
    std::cout << "\n";
    return 0;
}
//print a float
extern "C" DLLEXPORT int printFloat_float(float X) {
    std::cout << " \n Out: " << X << "\n";
    return 0;
}

//get float input
extern "C" DLLEXPORT float inF() {
    float a = 0;
    std::cin >> a;
    return a;
}

extern "C" DLLEXPORT int inI() {
    int a = 0;
    std::cin >> a;
    return a;
}
//convert to string
extern "C" DLLEXPORT const char* toString_float(float a) {
    std::string* out =   new std::string(std::to_string(a));
    return out->data();
}
extern "C" DLLEXPORT const char* toString_int(int a) {
    std::string* out =   new std::string(std::to_string(a));
    return out->data();
}
//add two strings
extern "C" DLLEXPORT const char* concat_chars_chars(char* a,char* b) {
    std::string* out = new std::string((std::string(a)+std::string(b)));
    return out->data();
}

extern "C" DLLEXPORT float pow_float_float(float a, float b) {
    return pow(a,b);
}
