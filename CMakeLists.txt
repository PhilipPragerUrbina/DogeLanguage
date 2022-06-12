cmake_minimum_required(VERSION 3.22)
project(program)

set(CMAKE_CXX_STANDARD 17)

#find llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

#create llvm
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(program main.cpp Scanner.hpp File.hpp ErrorHandler.hpp Color.hpp Expression.hpp Parser.hpp Printers.hpp Interpreter.hpp Environment.hpp Callable.hpp Optimizer.hpp IRCompiler.hpp Analyzer.hpp)


#link llvm
llvm_map_components_to_libnames(
        llvm_libs
        Analysis
        Core
        ExecutionEngine
        InstCombine
        Object
        OrcJIT
        RuntimeDyld
        ScalarOpts
        Support
        TransformUtils
        native
        irreader
        AsmParser
        Interpreter
        CodeGen
        ARM
        X86
        WebAssembly
        VE
        SystemZ
        MSP430
        AMDGPU
        RISCV
        PowerPC
        Hexagon
        NVPTX
        XCore
        Lanai
        Sparc
        AVR
        BPF
        Mips
        AArch64

)
target_link_libraries(program ${llvm_libs})