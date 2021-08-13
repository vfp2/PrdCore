#include "stdafx.h"
#include "LpFilter.h"
#include "PrdSourcePcqng.h"
#include "Buffer.h"
#include <process.h>
#include <random>


PrdSourcePcqng::PrdSourcePcqng()
    : runnerThread(0)
    , endEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
    , initCount(10)
    , prevBit(0)
    , sourceBuffer(1000, false)
{
    SetBufferSizeMs(200);
    Start();
}


PrdSourcePcqng::~PrdSourcePcqng()
{
    Stop();
    CloseHandle(endEvent);
    endEvent = 0;
}


void PrdSourcePcqng::Start()
{
    // start millisecond timer
    mmTimerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (mmTimerEvent == NULL)
        throw "MilliTimer: CreateEvent FAILED";

    TIMECAPS timeCaps;
    timeGetDevCaps(&timeCaps, sizeof(timeCaps));
    if (timeCaps.wPeriodMin != 1)
        throw "MilliTimer: 1ms resolution not available";

    mmTimerId = timeSetEvent(1, 0, (LPTIMECALLBACK)mmTimerEvent, 1, TIME_PERIODIC | TIME_CALLBACK_EVENT_PULSE);
    if (mmTimerId == 0)
        throw "MilliTimer: Start FAILED";

    // Start the runner thread
    ResetEvent(endEvent);
    initCount = 10;
    runnerThread = (HANDLE)_beginthreadex(NULL, 0, PrdSourcePcqng::Runner, this, 0, NULL);
}


void PrdSourcePcqng::Stop()
{
    SetEvent(endEvent);
    SetEvent(mmTimerEvent);
    WaitForSingleObject(runnerThread, INFINITE);
    CloseHandle(runnerThread);
    runnerThread = 0;
    timeKillEvent(mmTimerId);
    CloseHandle(mmTimerEvent);
    mmTimerEvent = 0;
}


unsigned __stdcall PrdSourcePcqng::Runner(void* self)
{
    if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0)
        throw "PcqngCore: SetThreadAffinityMask FAILED";
    Sleep(1);

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
        throw "PcqngCore: SetThreadPriority FAILED";

    int phase = 0;
    int procCounter = 0;
    PrdSourcePcqng* obj = (PrdSourcePcqng*)self;
    uint64_t tscWord = 0;
    uint64_t prevTscWord = 0;
    int64_t tscDiff;
    LpFilter lpFilter;
    double qFactor;

    uint8_t  eBits;
    uint8_t correctedWords[72];
    unsigned int makeTargetCount = 0;

    if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0)
        throw "PcqngCore: SetThreadAffinityMask FAILED";
    Sleep(1);

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
        throw "PcqngCore: SetThreadPriority FAILED";

    while (true)
    {
        if (WaitForSingleObject(obj->endEvent, 0) == WAIT_OBJECT_0)
            break;


        // wait for multimedia timer fire
        DWORD waitResponse = WaitForSingleObject(obj->mmTimerEvent, INFINITE);
        if (waitResponse == WAIT_OBJECT_0)
        {
            // TSC Measurement
            __asm
            {
                rdtsc
                mov dword ptr[tscWord], eax
                mov dword ptr[tscWord]+4, edx
            }

            // TSC difference
            tscDiff = (tscWord-prevTscWord);
            prevTscWord = tscWord;

            // normal processing
            if (phase == 2) // normal processing phase
            {
                double filterRatio = (double)tscDiff/lpFilter.GetValue();

                if ((filterRatio > 1.05) || (filterRatio < 0.95))
                    lpFilter.Feed((double)tscDiff, 1000);
                else
                    lpFilter.Feed((double)tscDiff, 100);

                qFactor = lpFilter.GetValue()/33333;                       // Quantization
                eBits = (char)(((double)tscDiff / qFactor) + 0.5); // Entropic bits
            }
            else if (phase == 1) // ramp up phase
            {
                lpFilter.Feed((double)tscDiff, 100);
                if (++procCounter >= 1000)
                {
                    procCounter = 0;
                    phase = 2;
                }
            }
            else if (phase == 0) // initialization phase
            {
                lpFilter.Init((double)tscDiff);
                if (++procCounter >= 3)
                {
                    procCounter = 0;
                    phase = 1;
                }
            }
        }
        else
            throw "PcqngCore: MMTimer event FAILED";


        // corrector expansion
        for (int i=0; i<36; i++)
        {
            correctedWords[2*i] = 0;
            correctedWords[2*i+1] = 0;
            for (int b=0; b<7; b++)
            {
                uint8_t newBit = obj->lfsrCorrector.Next((eBits>>(6-b))&0x1);
                correctedWords[2*i] |= (newBit<<7);
                if (newBit == obj->prevBit)
                    correctedWords[2*i+1] |= 0x80;
                obj->prevBit = newBit;
                correctedWords[2*i] >>= 1;
                correctedWords[2*i+1] >>= 1;
            }
            correctedWords[2*i] |= 0x80;
        }
        obj->sourceBuffer.Write((char*)correctedWords, 72);
    }

    return 0;
}


double PrdSourcePcqng::GetRawDataRate()
{
    return 252000.0;
}


vector<uint8_t> PrdSourcePcqng::GetRawData(int byteCount)
{
    vector<uint8_t> rawData(byteCount);
    sourceBuffer.Read(rawData, byteCount);

    return rawData;
}


string PrdSourcePcqng::GetId()
{
    return "PCQNG";
}


int PrdSourcePcqng::SetRawBufferSize(int requestedRawBufferSize)
{
    sourceBuffer.SetSize(requestedRawBufferSize);

    return requestedRawBufferSize;
}


double PrdSourcePcqng::SetBufferSizeMs(double bufferSizeMs)
{
    int newSize = (int)(2*GetRawDataRate()*bufferSizeMs/7000.0 + 0.5);
    sourceBuffer.SetSize(newSize);

    return (newSize*7000.0/(2*GetRawDataRate()));
}


int PrdSourcePcqng::SendCommands(vector<uint8_t> writeBuffer)
{
    return 0;
}
