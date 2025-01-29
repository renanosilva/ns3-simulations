#include "node-asleep-exception.h"

namespace ns3
{

// Constructor accepts a const char* that is used to set
// the exception message
NodeAsleepException::NodeAsleepException(const char* msg)
    : message(msg)
{
}

// Override the what() method to return our message
const char* NodeAsleepException::what() const throw()
{
    return message.c_str();
}

}