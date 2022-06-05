#include <iostream>
#include<string>
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

//print a char from a float
extern "C" DLLEXPORT int putF(float X) {
    fputc((char)X, stdout);
    return 0;
}
//print a constant string
extern "C" DLLEXPORT int printC(char* chars) {
    std::cout << chars;
    return 0;
}
//print a float
extern "C" DLLEXPORT int printFloat(float X) {
    std::cout << " \n Out: " << X << "\n";
    return 0;
}
//get float input
extern "C" DLLEXPORT float inF() {
    float a = 0;
    std::cin >> a;
    return a;
}
extern "C" DLLEXPORT const char* toString(float a) {
    std::string out =  std::to_string(a);

    char* ptr = new char[out.size() + 1]; // +1 for terminating NUL

    // Copy source string in dynamically allocated string buffer
    strcpy(ptr, out.c_str());

    // Return the pointer to the dynamically allocated buffer
    return ptr;
}

extern "C" DLLEXPORT const char* concat(char* a,char* b) {
    std::string out = (std::string(a)+std::string(b));

    char* ptr = new char[out.size() + 1]; // +1 for terminating NUL

    // Copy source string in dynamically allocated string buffer
    strcpy(ptr, out.c_str());

    // Return the pointer to the dynamically allocated buffer
    return ptr;
}

//main function
extern "C" {
int DogeMain();
}
//run
int main() {
    std::cout << "\n Running Doge:\n";
    int out = DogeMain();
    std::cout << "Doge out:" <<out << "\n";
    return out;
}