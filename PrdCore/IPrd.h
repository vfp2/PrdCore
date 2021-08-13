#pragma once


#include "IRng.h"
#include "PcqngRng.h"

#include <string>
#include <memory>

using namespace std;


class IPrd
{
public:
	IPrd(IRng* rng, PcqngRng* pcqng);

    string GetId();
    Buffer* GetRngBuffer();
    Buffer* GetTargetBuffer();
    int GetBitRate();
    virtual void GenPrdResult(int count, vector<double>& psiData, vector<double>& targetData) = 0;
    int GetPsiMode();
    void SetPsiMode(int psiMode);
    bool IsAcProcessing();
    void SetAcProcessing(bool doAcProcessing);

protected:
    int bitRate;
    string id;
    IRng* rng;
    PcqngRng* pcqng;
    int targetMode;
    bool isAcProcessing;
};
