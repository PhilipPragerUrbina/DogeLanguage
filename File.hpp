//
// Created by Philip on 5/20/2022.
//

#ifndef PROGRAM_FILE_HPP
#define PROGRAM_FILE_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

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
            std::cerr << "Can not open file: " << filename << "\n";
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