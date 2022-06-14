//
// Created by Philip on 5/20/2022.
//

#ifndef PROGRAM_FILE_HPP
#define PROGRAM_FILE_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#elif
#include <unistd.h>
#endif
//helper function for getting executable directory
std::string getExeDirectory(std::string filename)
{
#ifdef _WIN32
    // Windows specific
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW( NULL, szPath, MAX_PATH );
#else
    // Linux specific
    char szPath[PATH_MAX];
    ssize_t count = readlink( "/proc/self/exe", szPath, PATH_MAX );
    if( count < 0 || count >= PATH_MAX )
        return {}; // some error
    szPath[count] = '\0';
#endif
    return (std::filesystem::path{ szPath }.parent_path() / "").string() + "/" + filename; // to finish the folder path with (back)slash
}

//file class for handling multiple files
class File {
public:
    File(std::string filename){
        //create file
        m_filename = filename;
        std::cout << "Opening file: " << filename << "\n";
        m_stream = std::ifstream(filename);

        //read file
        std::string line;
        if (m_stream.is_open())
        {
            while ( getline (m_stream,line) )
            {
                //record data
                m_lines.push_back(line + " ");
                m_length++;
            }
            m_stream.close();
            //file read successfully
            m_read = true;
            std::cout << "File read successfully" << "\n";

        }else{
            std::cout << "Can not open file: " << filename << "\n";
        }
    }
    //is file read good
    const bool read(){
        return  m_read;
    }
    //return number of lines
    const int getLength(){
        return m_length;
    }
    //get all lines
    std::vector<std::string > getData(){
        return m_lines;
    }
    //get specific lines
    const std::string getLine(const int i){
        return m_lines[i];
    }
    const std::string operator[](const int i) {
        return m_lines[i];
    }
    const std::string getName(){
        return m_filename;
    }

private:
    //file data
    std::string m_filename;
    std::ifstream m_stream;
    std::vector<std::string> m_lines;
    //you can also call m_lines.size()
    int m_length = 0;
    bool m_read = false;
};

#endif PROGRAM_FILE_HPP