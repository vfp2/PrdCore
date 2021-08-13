#pragma once


#include "IPrdSource.h"
#include "FtdiPrdDevice.h"

#include <Windows.h>
#include <FTD2XX.h>
#include <vector>

using namespace std;


// manages FTDI PRD device(s) as a single RNG source
class PrdSourceFtdi : public IPrdSource
{
public:
    PrdSourceFtdi(string id);
    ~PrdSourceFtdi();

    virtual double GetRawDataRate();
    virtual vector<uint8_t> GetRawData(int byteCount);
    virtual string GetId();
    int SetRawBufferSize(int requestedRawBufferSize);
    virtual double SetBufferSizeMs(double requestedBufferSizeMs);
    virtual int SendCommands(vector<uint8_t> writeBuffer);

    static vector<string> ListSources();

private:
    void Start();
    void Stop();

    unsigned arraySize;
    vector<FtdiPrdDevice*> ftdiArray;
    string id;
    Buffer sourceBuffer;

friend class PrdSourceFtdiArrayTest;
};
