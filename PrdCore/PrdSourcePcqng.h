#pragma once


#include "IPrdSource.h"
#include "LfsrCorrector.h"
#include "Buffer.h"

#include <Mmsystem.h>


class PrdSourcePcqng :
    public IPrdSource
{
public:
    PrdSourcePcqng();
    ~PrdSourcePcqng();

    virtual double GetRawDataRate();
    virtual vector<uint8_t> GetRawData(int byteCount);
    virtual string GetId();
    virtual int SetRawBufferSize(int requestedRawBufferSize);
    virtual double SetBufferSizeMs(double requestedBufferSizeMs);
    virtual int SendCommands(vector<uint8_t> writeBuffer);

private:
    void Start();
    void Stop();
    static unsigned __stdcall Runner(void* self);

private:
    HANDLE runnerThread;
    HANDLE endEvent;
    LfsrCorrector lfsrCorrector;
    int initCount;
    HANDLE mmTimerEvent;
    MMRESULT mmTimerId;
    uint8_t prevBit;
    Buffer sourceBuffer;
};
