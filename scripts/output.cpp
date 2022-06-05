#include <iostream>
//example linking with compiled doge
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

//print a char from a float
extern "C" DLLEXPORT float putF(float X) {
    fputc((char)X, stdout);
    return 0;
}
//print a constant string
extern "C" DLLEXPORT float printC(char* chars) {
    std::cout << chars;
    return 0;
}
//print a float
extern "C" DLLEXPORT float printFloat(float X) {
    std::cout << " \n Out: " << X << "\n";
    return 0;
}
//get float input
extern "C" DLLEXPORT float inF() {
    float a = 0;
    std::cin >> a;
    return a;
}


//main function
extern "C" {
float DogeMain();
}
//run
int main() {
    std::cout << "\n Running Doge:\n";
    float out = DogeMain();
    std::cout << "Doge out:" <<out << "\n";
    return (int)out;
}