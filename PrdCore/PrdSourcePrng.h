#pragma once


#include "IPrdSource.h"
#include "Buffer.h"

#include <random>
#include <Mmsystem.h>
#include <Windows.h>


class PrdSourcePrng :
    public IPrdSource
{
public:
    PrdSourcePrng();
    ~PrdSourcePrng();

    virtual double GetRawDataRate();
    virtual vector<uint8_t> GetRawData(int byteCount);
    virtual string GetId();
    int SetRawBufferSize(int requestedRawBufferSize);
    virtual double SetBufferSizeMs(double requestedBufferSizeMs);
    virtual int SendCommands(vector<uint8_t> writeBuffer);

private:
    void Start();
    void Stop();
    static unsigned __stdcall Runner(void* self);

private:
    HANDLE runnerThread;
    HANDLE endEvent;
    int initCount;
    HANDLE mmTimerEvent;
    MMRESULT mmTimerId;
    mt19937 twister;
    uint8_t prevBit;
    Buffer sourceBuffer;
};
