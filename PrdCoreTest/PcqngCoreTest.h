#pragma once


#include "../PrdCore/PcqngRng.h"

extern int successes;
extern int failures;


class PcqngCoreTest
{
public:
    static void Test()
    {
        shared_ptr<Buffer> rngBuffer = shared_ptr<Buffer>(new Buffer(1000, false));
        shared_ptr<Buffer> targetBuffer = shared_ptr<Buffer>(new Buffer(1000, true));
        shared_ptr<PcqngRng> pcqng = shared_ptr<PcqngRng>(new PcqngRng("PCQNG", rngBuffer.get(), targetBuffer.get()));

        vector<uint8_t> testBuffer;

        // init phase
        vector<uint8_t> inBuffer;
        for (int i=0; i<50; i++)
            rngBuffer->Read(inBuffer, 1000);

        // time output
        LARGE_INTEGER pStart, pEnd;
        LARGE_INTEGER pFreq;
        QueryPerformanceFrequency(&pFreq);

        QueryPerformanceCounter(&pStart);
        for (int i=0; i<100; i++) {
            rngBuffer->Read(inBuffer, 1000);
            testBuffer.insert(testBuffer.end(), inBuffer.begin(), inBuffer.end());
        }
        QueryPerformanceCounter(&pEnd);

        double timeElapsed = (double)(pEnd.QuadPart-pStart.QuadPart) / (double)pFreq.QuadPart;
        double bitsGend = 100 * 7*1000;

        double bps = bitsGend / timeElapsed;
        double targetBps = pcqng->GetBitRate();

        // check that bps no more than 1% off
        double bpsRatio = bps/targetBps;
        if (bpsRatio < 1.0)
            bpsRatio = 1.0/bpsRatio;
        if (bpsRatio <= 1.01)
        {
            printf("PcqngRng BPS Rate Measured: %i   Target: %i\n", (int)(bps+0.5), (int)targetBps);
            successes++;
        }
        else
        {
            printf("ERROR: PcqngRng BPS Rate Measured: %i   Target: %i\n", (int)(bps+0.5), (int)targetBps);
            failures++;
        }

        // check that data bias no more than 4.0 standard deviations
        long oneCount = 0;
        long totalCount = testBuffer.size()*7;
        for (size_t i=0; i<testBuffer.size(); i++)
        {
            for (int j=0; j<7; j++)
                oneCount += (testBuffer[i]>>j)&0x1;
        }

        double z = (2.0*oneCount - totalCount) / sqrt(totalCount);
        if (abs(z) <= 4.0)
        {
            printf("PcqngRng Bias Z-Score Measured: %+1.2f of %i Bits\n", z, totalCount);
            successes++;
        }
        else
        {
            printf("ERROR: PcqngRng Bias Z-Score Measured: %+1.2f of %i Bits\n", z, totalCount);
            failures++;
        }


        // check the rate on targets
        testBuffer.clear();
        for (int i=0; i<2; i++)
            targetBuffer->Read(inBuffer, 550);

        // time output
        QueryPerformanceCounter(&pStart);
        for (int i=0; i<50; i++) {
            targetBuffer->Read(inBuffer, 10);
            testBuffer.insert(testBuffer.end(), inBuffer.begin(), inBuffer.end());
        }
        QueryPerformanceCounter(&pEnd);

        timeElapsed = (double)(pEnd.QuadPart-pStart.QuadPart) / (double)pFreq.QuadPart;
        bitsGend = 50 * 10;

        bps = bitsGend / timeElapsed;
        targetBps = 125;

        // check that bps no more than 1% off
        bpsRatio = bps/targetBps;
        if (bpsRatio < 1.0)
            bpsRatio = 1.0/bpsRatio;
        if (bpsRatio <= 1.01)
        {
            printf("PcqngRng Target Bits BPS Rate Measured: %i   Target: %i\n", (int)(bps+0.5), (int)targetBps);
            successes++;
        }
        else
        {
            printf("ERROR: PcqngRng Target Bits BPS Rate Measured: %i   Target: %i\n", (int)(bps+0.5), (int)targetBps);
            failures++;
        }

        // check that data bias no more than 4.0 standard deviations
        oneCount = 0;
        totalCount = testBuffer.size();
        for (size_t i=0; i<testBuffer.size(); i++)
        {
            for (int j=0; j<7; j++)
                oneCount += (testBuffer[i]>>j)&0x1;
        }

        z = (2.0*oneCount - totalCount) / sqrt(totalCount);
        if (abs(z) <= 4.0)
        {
            printf("PcqngRng Target Bits Bias Z-Score Measured: %+1.2f of %i Bits\n", z, totalCount);
            successes++;
        }
        else
        {
            printf("ERROR: PcqngRng Target Bits Bias Z-Score Measured: %+1.2f of %i Bits\n", z, totalCount);
            failures++;
        }
    }
};
