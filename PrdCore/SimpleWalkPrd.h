#pragma once

#include "IPrd.h"
#include "IRng.h"
#include "PcqngRng.h"

#include <random>

class SimpleWalkPrd :
    public IPrd
{
public:
    SimpleWalkPrd(IRng* rng, PcqngRng* pcqng);
//    ~SimpleWalkPrd();
    virtual void GenPrdResult(int count, vector<double>& psiData, vector<double>& targetData);
    void GenTrial(double& biasResult, double& acResult, uint8_t& target);
    int GetPsiMode();
    void SetPsiMode(int psiMode);

private:
    void CreateCountTable();
    
    int trialBytesCount;
    int countTable[256];

    LARGE_INTEGER pStamp;
    mt19937 tgen;
    uint8_t tBuff[2];
};

