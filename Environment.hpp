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
    int size(){
        return m_values.size();
    }
    Environment* copy()
    {
        Environment* new_env = new Environment;
        new_env->m_values = m_values;
        new_env->m_constants = m_constants;
        new_env->m_enclosing = m_enclosing;
        return new_env;
    }
    //return true if already defined
    bool define(std::string name, object value, bool constant = false){
        if (m_values.find(name) == m_values.end() ) {
            m_values[name] = value;
            m_constants[name] = constant;
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

    bool isConst(std::string name){
        return m_constants[name];
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
    //could be faster?
    std::unordered_map<std::string,object> m_values;
    std::unordered_map<std::string,bool> m_constants;
    //could be more memory efficient but slower?
    // std::map<std::string,object> m_values;
private:
    Environment* m_enclosing = nullptr;


};

#endif PROGRAM_ENVIRONMENT_HPP