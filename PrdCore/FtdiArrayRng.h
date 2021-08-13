// © 2012 Psigenics
//
// FtdiArrayRng: Manages array of FTDI devices as if it is one device
//
// 2012.09.28    pawilber        Creation: Manages array of FTDI devices as if one device
//                               But will also work on single device


#pragma once


#include "IRng.h"
#include "FtdiRng.h"

#include <Windows.h>
#include <FTD2XX.h>
#include <vector>


class Buffer;


// manages (or array) FTDI PRD device(s) as a single RNG source
class FtdiArrayRng : public IRng
{
public:
    FtdiArrayRng(string id, Buffer* rngBuffer);
    ~FtdiArrayRng();

public:
    // writes to eligible devices
    virtual int Write(vector<uint8_t> writeBuffer);
    // lists FTDI PRD sources
    static vector<string> ListSources();
    virtual int GetBitRate();
    virtual int SetBufferSizeMs(int bufferSizeMs);

private:
    // starts device(s)
    void Start();
    // stops device(s)
    void Stop();

    unsigned arraySize;
    // actual FTDI PRD device(s) constituing the array
    vector<FtdiRng*> ftdiArray;
    // id of this device or array
    string id;

friend class FtdiArrayRngTest;
};
