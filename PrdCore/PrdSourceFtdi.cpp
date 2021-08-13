// © 2013 Psigenics
//
// PrdSourceFtdi: IPrdSource implementation for FTDI devices
//
// 2013.02.28    pawilber        Standardizes output for 2 channel and 1 channel devices to 16 channel output format. Adds AC channel to 1 channel source.


#include "stdafx.h"
#include "PrdSourceFtdi.h"
#include "Buffer.h"
#include "stdint.h"

#include <string>
#include <process.h>


PrdSourceFtdi::PrdSourceFtdi(string id)
    : id(id)
    , arraySize((id.find("x")==9)? (id.at(10)-48) : 1)
    , sourceBuffer(1000, false)
{
    Start();
}


PrdSourceFtdi::~PrdSourceFtdi()
{
    for (unsigned i=0; i<arraySize; i++)
        delete ftdiArray[i];

    ftdiArray.clear();
    arraySize = 0;
}


void PrdSourceFtdi::Start()
{
    std::vector<string> sourceList = FtdiPrdDevice::ListSources();
    std::string prefix = id.substr(0, 4);
    int sourcesCount = sourceList.size();
    int k = 0;

    for (unsigned i=0; i<arraySize; i++)
    {
        // verify source availability
        while (1)
        {
            if (sourceList[k].find(prefix) == 0)
                break;
            if (++k == sourcesCount)
                throw "NO HARDWARE"; // did not find an available device
        }
        FtdiPrdDevice* prdFtdiDevice = new FtdiPrdDevice(sourceList[i], sourceBuffer);
        ftdiArray.push_back(prdFtdiDevice);
    }

    for (unsigned i=0; i<arraySize; i++)
        ftdiArray[i]->Start();
}


void PrdSourceFtdi::Stop()
{ }


std::vector<std::string> PrdSourceFtdi::ListSources()
{
    vector<string> sourceList = FtdiPrdDevice::ListSources();

    for (unsigned i=0; i<sourceList.size(); i++)
    {
        for (unsigned j=i+1; j<sourceList.size(); j++)
        {
            if (sourceList[j].find(sourceList[i].substr(0, 4)) == 0)
            {
                if (sourceList[i].length() == 8)
                    sourceList[i].append("SN");
                if (sourceList[i].find("SN") == 9)
                    sourceList[i].replace(9, 2, "x2");
                else if (sourceList[i].find("x") == 9)
                    sourceList[i].replace(10, 1, 1, (char)(sourceList[i].at(10)+1));
                sourceList.erase(sourceList.begin()+j);
                j--;
            }
        }
    }

    std::reverse(sourceList.begin(), sourceList.end());

    return sourceList;
}


int PrdSourceFtdi::SendCommands(vector<uint8_t> writeBuffer)
{
	DWORD bytesWritten = 0;
	if (id.find("PD") == 0)
    {
        DWORD returnBytesWritten = INT_MAX;
        for (unsigned i=0; i<arraySize; i++)
            bytesWritten = ftdiArray[i]->SendCommands(writeBuffer);

		return bytesWritten;
	}
	else
		return 0;
}


double PrdSourceFtdi::GetRawDataRate()
{
    double bitRate = arraySize * ftdiArray[0]->GetRawDataRate();

    return bitRate;
}


vector<uint8_t> PrdSourceFtdi::GetRawData(int byteCount)
{
    vector<uint8_t> rawData(byteCount);
    sourceBuffer.Read(rawData, byteCount);

    return rawData;
}


string PrdSourceFtdi::GetId()
{
    return id;
}


int PrdSourceFtdi::SetRawBufferSize(int requestedRawBufferSize)
{
    sourceBuffer.SetSize(requestedRawBufferSize);

    return requestedRawBufferSize;
}


double PrdSourceFtdi::SetBufferSizeMs(double bufferSizeMs)
{
    double secondsFraction = (double)bufferSizeMs/1000.0;
    int bufferSizeBytes = (int)ceil(secondsFraction * (double)GetRawDataRate() / 7.0);  // 7 bits per byte
    bufferSizeBytes *= 2; // bias and ac

    sourceBuffer.SetSize(bufferSizeBytes);
    return bufferSizeBytes;
}
