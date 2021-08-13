#pragma once


#include "IPrdProcessor.h"


class SimpleRandomWalkProcessor :
    public IPrdProcessor
{
public:
    SimpleRandomWalkProcessor();

    virtual int GetProcessorId(string* outId);
    virtual int SetTrialParams(int inPrdCoreHandle, int inPsiMode, double inRawGenBps, double inRequestedTrialLengthMs, double* outActualTrialLengthMs);
    virtual int GenTrial(double* outTrialResult);

private:
    int psiMode;
//    bool isAcProcessing;
    double rawGenBps;
    int trialByteCount;
    double trialLengthMs;
    int countTable[256];

    typedef int (*FnGetRawData)(int prdCoreHandle, int count, char* rawData);
    typedef int (*FnGetClairvoyanceTarget)(int inPrdCoreHandle, double* outClairvoyanceTarget);
    typedef int (*FnGenPrecognitionTarget)(int inPrdCoreHandle, double* outPrecognitionTarget);

    FnGetRawData GetRawData;
    FnGetClairvoyanceTarget GetClairvoyanceTarget;
    FnGenPrecognitionTarget GenPrecognitionTarget;

    HMODULE prdCoreLib;
    int prdCoreHandle;
};