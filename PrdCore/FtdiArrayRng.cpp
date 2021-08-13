// © 2012 Psigenics
//
// FtdiArrayRng: Manages array of FTDI devices as if it is one device
//
// 2012.09.28    pawilber        Creation: Manages array of FTDI devices as if one device
//                               But will also work on single device


#include "FtdiArrayRng.h"
#include "Buffer.h"
#include "stdint.h"

#include <string>
#include <process.h>


FtdiArrayRng::FtdiArrayRng(string id, Buffer* rngBuffer)
    : IRng(id, rngBuffer)
    , id(id)
{    
    SetBufferSizeMs(200);

    Start();
}


FtdiArrayRng::~FtdiArrayRng()
{
    for (unsigned i=0; i<arraySize; i++)
        delete ftdiArray[i];

    ftdiArray.clear();
    arraySize = 0;
}


void FtdiArrayRng::Start()
{
    std::vector<string> sourceList = FtdiRng::ListSources();
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
        FtdiRng* newFtdiRng = new FtdiRng(sourceList[i], rngBuffer);
        newFtdiRng->Start();
        ftdiArray.push_back(newFtdiRng);
    }
}


void FtdiArrayRng::Stop()
{
    for (unsigned i=0; i<arraySize; i++)
        ftdiArray[i]->Stop();
}


std::vector<std::string> FtdiArrayRng::ListSources()
{
    vector<string> sourceList = FtdiRng::ListSources();

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


int FtdiArrayRng::Write(vector<uint8_t> writeBuffer)
{
	DWORD bytesWritten = 0;
	if (id.find("PD") == 0)
    {
        DWORD returnBytesWritten = INT_MAX;
        for (unsigned i=0; i<arraySize; i++)
        {
            bytesWritten = ftdiArray[i]->Write(writeBuffer);
            if (bytesWritten < returnBytesWritten)
                returnBytesWritten = bytesWritten;

            if ((writeBuffer[0]&0xF0) == 0x30)
                SetBufferSizeMs(400);
            else
                SetBufferSizeMs(200);
        }

		return bytesWritten;
	}
	else
		return 0;
}


int FtdiArrayRng::GetBitRate()
{
    int bitRate = 0;

    // find our bitrate
    if (id.find("QWR1") == 0)
        bitRate = 875000;
    if (id.find("QWR2") == 0)
        bitRate = 875000;
    if (id.find("QWP4") == 0)
        bitRate = 250000;
    if (id.find("QWR4") == 0)
        bitRate = 100000;
    if (id.find("PD")==0 && id.find("QK")==4)
    {
        bitRate = (id[6]-'0') * 100000;
        bitRate += (id[7]-'0') * 10000;
        bitRate += (id[8]-'0') * 1000;
    }
    if (id.find("PD")==0 && id.find("QM")==4)
    {
        bitRate = (id[6]-'0') * 1000000;
        bitRate += (id[7]-'0') * 100000;
        bitRate += (id[8]-'0') * 10000;
    }

    // bitrate if array
    arraySize = 1;
    if (id.find("x") == 9)
    {
        arraySize = (id.at(10)-48);
        bitRate *= arraySize;
    }

    return bitRate;
}


int FtdiArrayRng::SetBufferSizeMs(int bufferSizeMs)
{
    double secondsFraction = (double)bufferSizeMs/1000.0;
    int bufferSizeBytes = (int)ceil(secondsFraction * (double)GetBitRate() / 7.0);  // 7 bits per byte

    rngBuffer->SetSize(bufferSizeBytes);
    return bufferSizeBytes;
}