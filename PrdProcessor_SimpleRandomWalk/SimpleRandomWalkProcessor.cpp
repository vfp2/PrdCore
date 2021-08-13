#include "stdafx.h"
#include "SimpleRandomWalkProcessor.h"


SimpleRandomWalkProcessor::SimpleRandomWalkProcessor()
{
    prdCoreLib = GetModuleHandleA("PrdCore");
    GetRawData = (FnGetRawData)GetProcAddress(prdCoreLib, "GetRawData");
    GetClairvoyanceTarget = (FnGetClairvoyanceTarget)GetProcAddress(prdCoreLib, "GetClairvoyenceTarget");
    GenPrecognitionTarget = (FnGenPrecognitionTarget)GetProcAddress(prdCoreLib, "GetPrecognitionTarget");

    // create one's count table of 7 lsbs
    for (int i=0; i<=0xFF; i++)
    {
        countTable[i] = i&0x1;
        for (int j=1; j<7; j++)
            countTable[i] += (i>>j)&0x1;
    }
}


int SimpleRandomWalkProcessor::GetProcessorId(string* outId)
{
    *outId = "SimpleRandomWalkProcessor";

    return 0;
}


int SimpleRandomWalkProcessor::SetTrialParams(int inPrdCoreHandle, int inPsiMode, double inRawGenBps, double inRequestedTrialLengthMs, double* outActualTrialLengthMs)
{
    this->prdCoreHandle = inPrdCoreHandle;
    this->psiMode = inPsiMode;
    this->rawGenBps = inRawGenBps;

    // we want to create an odd number of bits per trial to never have a zero outcome
    // data bits come in bytes with 7 bits of info
    trialByteCount = (int)(((inRequestedTrialLengthMs/1000)*rawGenBps) / 7);
    if ((trialByteCount%2) == 0)
        trialByteCount++;
    trialLengthMs = 1000 * 7*trialByteCount / rawGenBps;
    *outActualTrialLengthMs = trialLengthMs;

    return 0;
}


int SimpleRandomWalkProcessor::GenTrial(double* outTrialResult)
{
//    vector<uint8_t> trialBytes;

    int byteIndex = 0;
    int biasOnesCount = 0;
    int biasBytesCount = 0;
    int acOnesCount = 0;
    int acBytesCount = 0;
//    if (!isAcProcessing)
//        acBytesCount = trialByteCount;

    uint8_t* trialBytes = new uint8_t[2*trialByteCount]; 

    while (biasBytesCount<trialByteCount || acBytesCount<trialByteCount)
    {
        int bytesNeeded = trialByteCount - biasBytesCount;
//        if (isAcProcessing == true)
        bytesNeeded += trialByteCount - acBytesCount;

//        trialBytes.resize(bytesNeeded);
//        char* tBytes = (char*)&trialBytes[0];
        GetRawData(prdCoreHandle, bytesNeeded, (char*)trialBytes);

        for (byteIndex=0; byteIndex<bytesNeeded; byteIndex++)
        {
            if (trialBytes[byteIndex] >= 0x80) // bias byte
            {
                if (biasBytesCount < trialByteCount)
                {
                    biasOnesCount += countTable[trialBytes[byteIndex]];
                    biasBytesCount++;
                }
            }
            else // ac byte
            {
                if (acBytesCount < trialByteCount)
                {
                    acOnesCount += countTable[trialBytes[byteIndex]];
                    acBytesCount++;
                }
            }
        }
    }

    delete[] trialBytes;

    double biasResult = (2*biasOnesCount - trialByteCount*7) / sqrt(trialByteCount*7);

    double acResult = (2*acOnesCount - trialByteCount*7) / sqrt(trialByteCount*7);

    *outTrialResult = (biasResult+acResult) / sqrt(2);

    double target = 1;
    if (psiMode == 2)
        target = GetClairvoyanceTarget(prdCoreHandle, &target);
    if (psiMode == 3)
        target = GenPrecognitionTarget(prdCoreHandle, &target);

    *outTrialResult *= target;

    return 0;
}
