#pragma once


#include "Buffer.h"

#include <string>
#include <memory>

using namespace std;


class IRng
{
public:
	IRng(string id, Buffer* rngBuffer);
    virtual ~IRng();

    string GetId();
    Buffer* GetRngBuffer();
    virtual int Write(vector<uint8_t> writeBuffer) = 0;
    virtual int SetBufferSizeMs(int bufferSizeMs) = 0;
    virtual int GetBitRate() = 0;

protected:
    string id;
    Buffer* rngBuffer;
};
