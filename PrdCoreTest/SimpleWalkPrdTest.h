#include "../PrdCore/PcqngRng.h"
#include "../PrdCore/SimpleWalkPrd.h"


class SimpleWalkPrdTest
{
public:
    static void Test()
    {
        shared_ptr<Buffer> rngBuffer = shared_ptr<Buffer>(new Buffer(1000, false));
        shared_ptr<Buffer> targetBuffer = shared_ptr<Buffer>(new Buffer(1000, true));

        shared_ptr<PcqngRng> pcqng = shared_ptr<PcqngRng>(new PcqngRng("PCQNG", rngBuffer.get(), targetBuffer.get()));

        IPrd* prd = new SimpleWalkPrd(pcqng.get(), pcqng.get());

        int bitRate = prd->GetBitRate();

        // time output
        LARGE_INTEGER pStart, pEnd;
        LARGE_INTEGER pFreq;
        QueryPerformanceFrequency(&pFreq);

        vector<double> psiData;
        vector<double> targetData;

        // init phase
        prd->GenPrdResult(1000, psiData, targetData);

        QueryPerformanceCounter(&pStart);
        for (int i=0; i<50; i++)
        prd->GenPrdResult(100, psiData, targetData);
        QueryPerformanceCounter(&pEnd);

        double timeElapsed = (double)(pEnd.QuadPart-pStart.QuadPart) / (double)pFreq.QuadPart;
        double bitsGend = 5000;

        double bps = bitsGend / timeElapsed;
        double targetBps = 970;

        // check that bps no more than 1% off
        double bpsRatio = bps/targetBps;
        if (bpsRatio < 1.0)
            bpsRatio = 1.0/bpsRatio;
        if (bpsRatio <= 1.01)
        {
            printf("SimpleWalkPrd Trial Rate Measured: %i   Target: %i\n", (int)(bps+0.5), (int)targetBps);
            successes++;
        }
        else
        {
            printf("ERROR: SimpleWalkPrd Trial Rate Measured: %i   Target: %i\n", (int)(bps+0.5), (int)targetBps);
            failures++;
        }

        pcqng->Stop();
    }
};