#include "SimpleWalkPrd.h"

#pragma comment (lib, "winmm")


SimpleWalkPrd::SimpleWalkPrd(IRng* rng, PcqngRng* pcqng)
    : IPrd(rng, pcqng)
{
    CreateCountTable();
    trialBytesCount = (int)(rng->GetBitRate()/7000.0 + 0.5);
    if (trialBytesCount%2 == 0)
        trialBytesCount++;
}


void SimpleWalkPrd::GenPrdResult(int count, vector<double>& psiData, vector<double>& targetData)
{
    if (!isAcProcessing)
        psiData.resize(count);
    else
        psiData.resize(2*count);
    targetData.resize(count);

    double biasResult;
    double acResult;
    uint8_t target;

    for (int i=0; i<count; ++i)
    {
        GenTrial(biasResult, acResult, target);

        psiData[i] = biasResult;
        if (isAcProcessing)
            psiData[i+count] = acResult;
        targetData[i] = target;
	}
}


void SimpleWalkPrd::GenTrial(double& biasResult, double& acResult, uint8_t& target)
{
    vector<uint8_t> trialBytes;
    vector<uint8_t> targetByte;

    Buffer* rngBuffer = rng->GetRngBuffer();
    Buffer* targetBuffer = pcqng->GetTargetBuffer();

    int byteIndex = 0;
    int biasOnesCount = 0;
    int biasBytesCount = 0;
    int acOnesCount = 0;
    int acBytesCount = 0;
    if (!isAcProcessing)
        acBytesCount = trialBytesCount;

    while (biasBytesCount<trialBytesCount || acBytesCount<trialBytesCount)
    {
        int bytesNeeded = trialBytesCount - biasBytesCount;
        if (isAcProcessing == true)
            bytesNeeded += trialBytesCount - acBytesCount;

        rngBuffer->Read(trialBytes, bytesNeeded);
        for (byteIndex=0; byteIndex<bytesNeeded; byteIndex++)
        {
            if (trialBytes[byteIndex] >= 0x80) // bias byte
            {
                if (biasBytesCount < trialBytesCount)
                {
                    biasOnesCount += countTable[trialBytes[byteIndex]];
                    biasBytesCount++;
                }
            }
            else // ac byte
            {
                if (acBytesCount < trialBytesCount)
                {
                    acOnesCount += countTable[trialBytes[byteIndex]];
                    acBytesCount++;
                }
            }
        }
    }

    double z = (2*biasOnesCount - trialBytesCount*7) / sqrt(trialBytesCount*7);
    biasResult = z;

    acResult = 0;
    if (isAcProcessing)
    {
        double z = (2*acOnesCount - trialBytesCount*7) / sqrt(trialBytesCount*7);
        acResult = z;
    }

    if (targetMode <= 1) // no targets or PK
        target = 1;
    else if (targetMode == 2) // PC
    {
        QueryPerformanceCounter(&pStamp);
        tgen.seed(pStamp.LowPart);
        target = tgen()&0x1;
    }
    else if (targetMode == 3) // CV
    {
        target = tBuff[0];
        tBuff[0] = tBuff[1];
        QueryPerformanceCounter(&pStamp);
        tgen.seed(pStamp.LowPart);
        tBuff[1] = tgen()&0x1;
    }
}


void SimpleWalkPrd::CreateCountTable()
{
    for (int i=0; i<=0xFF; i++)
    {
        countTable[i] = i&0x1;
        for (int j=1; j<7; j++)
            countTable[i] += (i>>j)&0x1;
    }
}
