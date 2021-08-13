#pragma once
#include "IRng.h"
#include <random>

class PrngRng :
    public IRng
{
public:
    PrngRng(string id, Buffer* rngBuffer);
    ~PrngRng();

    void Start();
    void Stop();
    void ClearBuffer();
    virtual int Write(vector<uint8_t> writeBuffer);
    virtual int SetBufferSizeMs(int bufferSizeMs);
    virtual int GetBitRate();

private:
    static unsigned __stdcall Runner(void* self);

private:
    HANDLE runnerThread_;
    HANDLE endEvent_;
    int initCount_;
    HANDLE mmTimerEvent_;
    MMRESULT mmTimerId_;
    mt19937 twister;
};

