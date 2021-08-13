// © 2012 Psigenics
//
// FtdiRng: Interface of a single FTDI PRD device
//
// 2012.10.03    pawilber        Port to VC++ 2012


#pragma once


#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <Windows.h>

#include "IRng.h"
#include "Buffer.h"
#include "FTD2XX.h"

using namespace std;


class FtdiRng :
	public IRng
{
public:
	FtdiRng(string id, Buffer* rngBuffer);
	~FtdiRng();

    virtual void Start();
    virtual void Stop();
    virtual int Write(vector<uint8_t> writeBuffer);
    static vector<string> ListSources();
    virtual int GetBitRate();
    virtual int SetBufferSizeMs(int bufferSizeMs);

private:
    static unsigned __stdcall Runner(void* self);
    static unsigned __stdcall RunnerQWR4(void* self);

    FT_HANDLE ftHandle;
    HANDLE runnerThread;
    HANDLE endEvent;
    HANDLE ftdiEvent;
};
