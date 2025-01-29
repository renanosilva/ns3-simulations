#include "node-depleted-exception.h"

namespace ns3
{

// Constructor accepts a const char* that is used to set
// the exception message
NodeDepletedException::NodeDepletedException(const char* msg)
    : message(msg)
{
}

// Override the what() method to return our message
const char* NodeDepletedException::what() const throw()
{
    return message.c_str();
}

}