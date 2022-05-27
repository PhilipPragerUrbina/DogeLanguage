//
// Created by Philip on 5/23/2022.
//

#ifndef PROGRAM_ENVIRONMENT_HPP
#define PROGRAM_ENVIRONMENT_HPP
#include "Expression.hpp"
#include <map>
class Environment {

public:

    Environment(Environment* enclosing = nullptr){
        m_enclosing = enclosing;
    }

    //return true if already defined
    bool define(std::string name, object value){
        if (m_values.find(name) == m_values.end() ) {
            m_values[name] = value;
            return false;
        }
        return true;
    }
    //return true if defined
    bool assign(std::string name, object value){
        if (m_values.find(name) == m_values.end() ) {
            if (m_enclosing != nullptr) return m_enclosing->assign(name,value);
            return false;
        }
        m_values[name] = value;
        return true;
    }

    object getValue(std::string name){
        if (m_values.find(name) == m_values.end() ) {
            if (m_enclosing != nullptr) return m_enclosing->getValue(name);
            return null_object();
        }else{
            return m_values[name];
        }



    }

    Environment* getScope(std::string name){
        if (m_values.find(name) == m_values.end() ) {
            if (m_enclosing != nullptr) return m_enclosing->getScope(name);
            return nullptr;
        }else{
            return this;
        }



    }
private:
    Environment* m_enclosing = nullptr;
    //TODO test performance here
    //could be faster?
    std::unordered_map<std::string,object> m_values;
    //could be more memory efficient but slower?
   // std::map<std::string,object> m_values;
};

#endif PROGRAM_ENVIRONMENT_HPP