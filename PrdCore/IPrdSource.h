#pragma once


#include <string>
#include <vector>
#include <stdint.h>

using namespace std;


class IPrdSource
{
public:
    virtual vector<uint8_t> GetRawData(int byteCount) = 0;
    virtual double GetRawDataRate() = 0;
    virtual int SetRawBufferSize(int requestedRawBufferSize) = 0;
    virtual string GetId() = 0;
//    virtual double SetBufferSizeMs(double requestedBufferSizeMs) = 0;
//    virtual int SendCommands(vector<uint8_t> writeBuffer) = 0;
};
