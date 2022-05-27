cmake_minimum_required(VERSION 3.22)
project(program)

set(CMAKE_CXX_STANDARD 17)

add_executable(program main.cpp Scanner.hpp File.hpp ErrorHandler.hpp Color.hpp Expression.hpp Parser.hpp Printers.hpp Interpreter.hpp Environment.hpp Callable.hpp)
