#pragma once
#include "IRng.h"
#include "LfsrCorrector.h"

class PcqngRng :
    public IRng
{
public:
    PcqngRng(string id, Buffer* rngBuffer, Buffer* targetBuffer);
    ~PcqngRng();

    void Start();
    void Stop();
    void ClearBuffer();
    virtual int Write(vector<uint8_t> writeBuffer);
    virtual int SetBufferSizeMs(int bufferSizeMs);
    Buffer* GetTargetBuffer();
    virtual int GetBitRate();

private:
    static unsigned __stdcall Runner(void* self);

private:
    HANDLE runnerThread_;
    HANDLE endEvent_;
    LfsrCorrector lfsrCorrector;
    LfsrCorrector targetCorrector;
    int initCount_;
    Buffer* targetBuffer;
    HANDLE mmTimerEvent_;
    MMRESULT mmTimerId_;
};

