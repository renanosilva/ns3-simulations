#ifndef NODE_DEPLETED_EXCEPTION_H
#define NODE_DEPLETED_EXCEPTION_H

#include <exception>
#include <iostream>
#include <string>

using namespace std;

namespace ns3
{

/** Exceção que é lançada quando um nó está em modo DEPLETED. */
class NodeDepletedException : public exception {

private:
    string message;

public:
    // Constructor accepts a const char* that is used to set
    // the exception message
    NodeDepletedException(const char* msg);

    // Override the what() method to return our message
    const char* what() const throw();
};

}

#endif