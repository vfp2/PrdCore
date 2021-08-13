#pragma once


#include <stdint.h>
#include <vector>
#include <string>

using namespace std;


class IPrdProcessor
{
public:
    virtual int GetProcessorId(string* outId) = 0;
    virtual int SetTrialParams(int inPrdCoreHandle, int inPsiMode, double inRawGenBps, double inRequestedTrialLengthMs, double* outActualTrialLengthMs) = 0;
    virtual int GenTrial(double* outTrialResult) = 0;
};