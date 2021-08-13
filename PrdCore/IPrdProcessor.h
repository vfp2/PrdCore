#pragma once


#include "Buffer.h"

#include <memory>

using namespace std;


class IPrdProcessor
{
public:
    virtual double SetOutputRate(double outputRate) = 0;

protected:
    Buffer* rngBuffer;
    Buffer* targetBuffer;
};
