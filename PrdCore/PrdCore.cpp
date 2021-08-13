#include "stdafx.h"
#include "PrdCore.h"
#include "IPrdProcessor.h"
#include "PrdSourceFtdi.h"
#include "PrdSourcePcqng.h"
#include "PrdSourcePrng.h"

#include <math.h>
#include <time.h>
#include <map>
#include <random>

using namespace std;

int ListSources(char* outPrdSources);
int SetSource(int inPrdCoreHandle, char* inPrdSource);


struct PrdCoreObject
{
    PrdCoreObject() : prdSource(nullptr), prdProcessor(nullptr), trialLengthMs(20), mode(0) { }

    IPrdSource* prdSource;
    IPrdProcessor* prdProcessor;
    int mode;
    double clairvoyanceTargets[2];

    double trialLengthMs;
    double rawGenBps;

    HMODULE prdProcessorLib;
};


map<int, PrdCoreObject*> prdCoreHandles_;
mt19937 targetGenerator_;


int Open(char* inLicenseId)
{
    int idLen = (int)strlen(inLicenseId);

    int pwrdLen = 9;
    int tStampLen = 3;
    int userLen = idLen - pwrdLen - tStampLen - 2;
    if (userLen < 3)
        return (-1);

    char* userId = inLicenseId;
    char* tStamp = userId + userLen + 1;
    char* password = userId + userLen + tStampLen + 2;

    int userIndex = 0;
    // this is the dummy key
	char* key = "© 2013 Psigenics Corporation";
    // this is the real key
    key = "© 2007 Psigenics Corporation";

    // the prdHandle will come out to 0 before assignment, if the licensing check process does not shortcircuted, which is the general first level hack attempt.
    int prdCoreHandle = 9;

    userIndex = 0;
    for (int i=0; i<9; i++)
    {
        unsigned val = 0;
        if (i < 3)
            val = abs((int)((unsigned)password[i]-97) - (int)((unsigned)tStamp[i]%26) + 26)%26;
        else
        {
            val = abs((int)((unsigned)password[i]-97) - (int)((unsigned)userId[userIndex]%26) + 26)%26;
            userIndex++;
            userIndex %= userLen;
        }
        if (val != (unsigned)key[i]%26)
        {
			Sleep(1000);
            return (-1);
        }

        prdCoreHandle--;
    }

    if (tStamp[2] != 'z')
    {
        time_t timeTNow;
        time(&timeTNow);
        tm localTNow;
        localtime_s(&localTNow, &timeTNow);
        if (((localTNow.tm_year%10) >= (tStamp[0]-97)) && ((localTNow.tm_mon) >= (tStamp[1]-97)) && ((localTNow.tm_mday) > (tStamp[2]-97)))
        {
			Sleep(1000);
            return (-1);
        }
    }

    // assign a unique prdHandle, after successful licensing, prdHandle should start at 0 for this process
    map<int, PrdCoreObject*>::iterator handleIter;
    do
    {
        prdCoreHandle++;
        handleIter = prdCoreHandles_.find(prdCoreHandle);

    } while (handleIter != prdCoreHandles_.end());


    PrdCoreObject* prdCoreObject = new PrdCoreObject();
    prdCoreHandles_.insert(std::make_pair(prdCoreHandle, new PrdCoreObject()));

    char prdSources[65536];
    ListSources(prdSources);
    SetSource(prdCoreHandle, prdSources);

//    char prdProcessors[65536];
//    ListProcessors(prdProcessors);
//    SetProcessor(prdCoreHandle, prdProcessors, 0, 40, &prdCoreObject->trialLengthMs);

//    double clairvoyanceTarget;
//    GetClairvoyanceTarget(prdCoreHandle, &clairvoyanceTarget);
//    GetClairvoyanceTarget(prdCoreHandle, &clairvoyanceTarget);
    
    return prdCoreHandle;
}


int Close(int inPrdCoreHandle)
{
    map<int, PrdCoreObject*>::iterator handleIter;
    handleIter = prdCoreHandles_.find(inPrdCoreHandle);
    if (handleIter == prdCoreHandles_.end())
        return -1;

    delete prdCoreHandles_[inPrdCoreHandle];
    prdCoreHandles_.erase(handleIter);

    return 0;
}


int ListSources(char* outPrdSources)
{
    vector<string> sourceList;
    vector<string> hardwareIndividualList;
    
    sourceList = PrdSourceFtdi::ListSources();

    hardwareIndividualList = FtdiPrdDevice::ListSources();
    for (unsigned i=0; i<hardwareIndividualList.size(); i++)
    {
        if (find(sourceList.begin(), sourceList.end(), hardwareIndividualList[i]) == sourceList.end())
            sourceList.push_back(hardwareIndividualList[i]);
    }

    sourceList.push_back("PCQNG");
    sourceList.push_back("PRNG");

    unsigned stringPos = 0;
    for (unsigned i=0; i<sourceList.size(); i++)
    {
		strcpy_s(&outPrdSources[stringPos], sourceList[i].length()+1, sourceList[i].c_str());
        stringPos += (unsigned)sourceList[i].length() + 1;
    }

    // double null termination
    outPrdSources[stringPos] = '\0';

    return sourceList.size();
}


int SetSource(int inPrdCoreHandle, char* inPrdSource)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    vector<string> sourceList;
    vector<string> hardwareIndividualList;

    sourceList = PrdSourceFtdi::ListSources();

    hardwareIndividualList = FtdiPrdDevice::ListSources();
    for (unsigned i=0; i<hardwareIndividualList.size(); i++)
    {
        if (find(sourceList.begin(), sourceList.end(), hardwareIndividualList[i]) == sourceList.end())
            sourceList.push_back(hardwareIndividualList[i]);
    }

    sourceList.push_back("PCQNG");
    sourceList.push_back("PRNG");

    if (find(sourceList.begin(), sourceList.end(), inPrdSource) == sourceList.end())
        return -1;

    if (prdCoreObject->prdSource != nullptr)
        delete prdCoreObject->prdSource;        

    if (strcmp(inPrdSource, "PCQNG") == 0)
    {
        prdCoreObject->prdSource = new PrdSourcePcqng();
    }
    else if (strcmp(inPrdSource, "PRNG") == 0)
    {
        prdCoreObject->prdSource = new PrdSourcePrng();
    }
    else
    {
        prdCoreObject->prdSource = new PrdSourceFtdi(string(inPrdSource));
    }

    return 0;
}


int GetSource(int inPrdCoreHandle, char* outPrdSource)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    string id = prdCoreObject->prdSource->GetId();
    strcpy_s(outPrdSource, id.length()+1, id.c_str());

    return 0;
}

/*
int ListProcessors(char* outPrdProcessors)
{
    typedef void* (*FnCreatePrdProcessor)();

    vector<string> processorsList;

    int processorsCount = 0;
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;

    hFind = FindFirstFileA("PrdProcessor*.dll", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
        return processorsCount;
    else 
    {
        HMODULE prdProcessorLib = LoadLibraryA(findFileData.cFileName);
        FnCreatePrdProcessor CreatePrdProcessor = (FnCreatePrdProcessor)GetProcAddress(prdProcessorLib, "CreatePrdProcessor");
        IPrdProcessor* prdProcessor = (IPrdProcessor*)CreatePrdProcessor();

        string id;
 
        prdProcessor->GetProcessorId(&id);
        processorsList.push_back(id);
        processorsCount++;

        delete prdProcessor;
        FreeLibrary(prdProcessorLib);
    }

    while (FindNextFileA(hFind, &findFileData) != 0) 
    {
        HMODULE prdProcessorLib = LoadLibraryA(findFileData.cFileName);
        FnCreatePrdProcessor CreatePrdProcessor = (FnCreatePrdProcessor)GetProcAddress(prdProcessorLib, "CreatePrdProcessor");
        IPrdProcessor* prdProcessor = (IPrdProcessor*)CreatePrdProcessor();
        
        string id;
 
        prdProcessor->GetProcessorId(&id);
        processorsCount++;

        delete prdProcessor;
        FreeLibrary(prdProcessorLib);
    }

    FindClose(hFind);

    unsigned stringPos = 0;
    for (unsigned i=0; i<processorsList.size(); i++)
    {
		strcpy_s(&outPrdProcessors[stringPos], processorsList[i].length()+1, processorsList[i].c_str());
        stringPos += (unsigned)processorsList[i].length() + 1;
    }

    // double null termination
    outPrdProcessors[stringPos] = '\0';

    return processorsCount;
}


int SetProcessor(int inPrdCoreHandle, char* inPrdProcessor, int inMode, double inRequestedTrialLengthMs, double* outActualTrialLengthMs)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    typedef void* (*FnCreatePrdProcessor)();

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;

    hFind = FindFirstFileA("PrdProcessor*.dll", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) 
        return -1;
    else 
    {
        HMODULE prdProcessorLib = LoadLibraryA(findFileData.cFileName);
        FnCreatePrdProcessor CreatePrdProcessor = (FnCreatePrdProcessor)GetProcAddress(prdProcessorLib, "CreatePrdProcessor");
        IPrdProcessor* prdProcessor = (IPrdProcessor*)CreatePrdProcessor();

        string id;
        prdProcessor->GetProcessorId(&id);
        
        if (id.find(inPrdProcessor) == 0)
        {
            if (prdCoreObject->prdProcessor != nullptr)
            {
                delete prdCoreObject->prdProcessor;
                FreeLibrary(prdCoreObject->prdProcessorLib);
            }

            prdCoreObject->prdProcessorLib = prdProcessorLib;
            prdCoreObject->prdProcessor = prdProcessor;
            prdCoreObject->mode = inMode;
            prdProcessor->SetTrialParams(inPrdCoreHandle, inMode, prdCoreObject->prdSource->GetBitRate(), prdCoreObject->trialLengthMs, &prdCoreObject->trialLengthMs);

            FindClose(hFind);
            return 0;
        }
        else
        {
            delete prdProcessor;
            FreeLibrary(prdProcessorLib);
        }
    }

    while (FindNextFileA(hFind, &findFileData) != 0) 
    {
        HMODULE prdProcessorLib = LoadLibraryA(findFileData.cFileName);
        FnCreatePrdProcessor CreatePrdProcessor = (FnCreatePrdProcessor)GetProcAddress(prdProcessorLib, "CreatePrdProcessor");
        IPrdProcessor* prdProcessor = (IPrdProcessor*)CreatePrdProcessor();
        
        string id;
        prdProcessor->GetProcessorId(&id);
        
        if (id.find(inPrdProcessor) == 0)
        {
            if (prdCoreObject->prdSource != nullptr)
            {
                delete prdCoreObject->prdProcessor;
                FreeLibrary(prdCoreObject->prdProcessorLib);
            }

            prdCoreObject->prdProcessor = prdProcessor;
            prdCoreObject->prdProcessorLib = prdProcessorLib;
            prdCoreObject->mode = inMode;
            prdProcessor->SetTrialParams(inPrdCoreHandle, inMode, prdCoreObject->prdSource->GetBitRate(), prdCoreObject->trialLengthMs, &prdCoreObject->trialLengthMs);

            FindClose(hFind);
            return 0;
        }
        else
        {
            delete prdProcessor;
            FreeLibrary(prdProcessorLib);
        }
    }

    FindClose(hFind);

    return -1;
}


int GetProcessor(int inPrdCoreHandle, char* outPrdProcessor, int* outMode, double* outTrialLengthMs)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    string processorId;
    prdCoreObject->prdProcessor->GetProcessorId(&processorId);
    string id = processorId;
    strcpy_s(outPrdProcessor, id.length(), id.c_str());
    *outMode = prdCoreObject->mode;
    *outTrialLengthMs = prdCoreObject->trialLengthMs;

    return 0;
}


int GenTrial(int inPrdCoreHandle, double* outTrialResult)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    return prdCoreObject->prdProcessor->GenTrial(outTrialResult);
}
*/

    
// raw data API

int GetRawData(int inPrdCoreHandle, int inRequestedByteCount, char* outRawData)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    vector<uint8_t> rawData;
    rawData = prdCoreObject->prdSource->GetRawData(inRequestedByteCount);
    memcpy_s(outRawData, inRequestedByteCount, &rawData[0], rawData.size());

    return rawData.size();
}


int GetRawDataRate(int inPrdCoreHandle, double* outRawDataRate)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    *outRawDataRate = prdCoreObject->prdSource->GetRawDataRate();

    return 0;
}


int SetRawBufferSize(int inPrdCoreHandle, int inRequestedRawBufferSize)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    int newRawBufferSize = prdCoreObject->prdSource->SetRawBufferSize(inRequestedRawBufferSize);

    return newRawBufferSize;
}


int GenRawTarget(int inPrdCoreHandle, char* outRawTarget)
{
    LARGE_INTEGER performanceCount;
    QueryPerformanceCounter(&performanceCount);
    targetGenerator_.seed(performanceCount.LowPart ^ targetGenerator_());

    unsigned int randWord32 = targetGenerator_();
    unsigned char randByte = (randWord32 ^ (randWord32>>8) ^ (randWord32>>16) ^ (randWord32>>24)) & 0xFF;
    char randBits2 = (randByte ^ (randByte>>2) ^ (randByte>>4) ^ (randByte>>6)) & 0x3;

    *outRawTarget = (randBits2 ^ (randBits2>>1))&0x1;

    return 0;
}


/*
int GetClairvoyanceTarget(int inPrdCoreHandle, double* outClairvoyanceTarget)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];
    
    LARGE_INTEGER performanceCount;
    QueryPerformanceCounter(&performanceCount);
    targetGenerator_.seed(performanceCount.LowPart);

    *outClairvoyanceTarget = 2*prdCoreObject->clairvoyanceTargets[0] - 1;
    prdCoreObject->clairvoyanceTargets[0] = prdCoreObject->clairvoyanceTargets[1];
    prdCoreObject->clairvoyanceTargets[1] = targetGenerator_()&0x1;

    return 0;
}


int GenPrecognitionTarget(int inPrdCoreHandle, double* outPrecognitionTarget)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];
    
    LARGE_INTEGER performanceCount;
    QueryPerformanceCounter(&performanceCount);
    targetGenerator_.seed(performanceCount.LowPart);

    *outPrecognitionTarget = 2*(targetGenerator_()&0x1) - 1;

    return 0;
}


int SendCommands(int inPrdCoreHandle, int inCommandBufferByteCount, char* inCommandBuffer)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[inPrdCoreHandle];

    vector<uint8_t> writeBuffer;
    for (int i=0; i<inCommandBufferByteCount; i++)
        writeBuffer.push_back(inCommandBuffer[i]);

    return prdCoreObject->prdSource->SendCommands(writeBuffer);
}
*/