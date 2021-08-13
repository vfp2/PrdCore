#pragma once


#include "Buffer.h"
#include "FTD2XX.h"

#include <string>
#include <memory>
#include <vector>
#include <Windows.h>

using namespace std;


class FtdiPrdDevice
{
public:
	FtdiPrdDevice(string id, Buffer& sourceBuffer);
	~FtdiPrdDevice();

    void Start();
    double GetRawDataRate();
    string GetId();
    int SendCommands(vector<uint8_t> writeBuffer);

    static vector<string> ListSources();

private:
    void Stop();
    static unsigned __stdcall PrdRunner(void* self);
//    void DetermineRates();

    int Chan2ToChan16(vector<uint8_t>& dataBuffer);
    int Chan1ToChan16(vector<uint8_t>& dataBuffer);

    string id;
    Buffer& sourceBuffer;

    FT_HANDLE ftHandle;
    HANDLE ftdiEvent;

    HANDLE runnerThread;
    HANDLE endEvent;

    int nativeChannelCount;
    uint32_t chan0Stream;
    int chan0StreamBitCount;
    uint32_t chan1Stream; 
    int chan1StreamBitCount;
};
