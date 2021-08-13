#include "LpFilter.h"
#include "PcqngRng.h"
#include "Buffer.h"
#include <process.h>
#include <random>


PcqngRng::PcqngRng(string id, Buffer* rngBuffer, Buffer* targetBuffer)
    : IRng(id, rngBuffer)
    , targetBuffer(targetBuffer)
    , runnerThread_(0)
    , endEvent_(CreateEvent(NULL, TRUE, FALSE, NULL))
    , initCount_(10)
{
    Start();
}


PcqngRng::~PcqngRng()
{
    Stop();
    CloseHandle(endEvent_);
    endEvent_ = 0;
}



void PcqngRng::Start()
{
    // start millisecond timer
    mmTimerEvent_ = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (mmTimerEvent_ == NULL)
        throw "MilliTimer: CreateEvent FAILED";

    TIMECAPS timeCaps;
    timeGetDevCaps(&timeCaps, sizeof(timeCaps));
    if (timeCaps.wPeriodMin != 1)
        throw "MilliTimer: 1ms resolution not available";

    mmTimerId_ = timeSetEvent(1, 0, (LPTIMECALLBACK)mmTimerEvent_, 1, TIME_PERIODIC | TIME_CALLBACK_EVENT_PULSE);
    if (mmTimerId_ == 0)
        throw "MilliTimer: Start FAILED";

    // Start the runner thread
    ResetEvent(endEvent_);
    initCount_ = 10;
    runnerThread_ = (HANDLE)_beginthreadex(NULL, 0, PcqngRng::Runner, this, 0, NULL);
}


void PcqngRng::Stop()
{
    SetEvent(endEvent_);
    SetEvent(mmTimerEvent_);
    WaitForSingleObject(runnerThread_, INFINITE);
    CloseHandle(runnerThread_);
    runnerThread_ = 0;
    timeKillEvent(mmTimerId_);
    CloseHandle(mmTimerEvent_);
    mmTimerEvent_ = 0;
}


void PcqngRng::ClearBuffer()
{
//    inBuffer_.Clear();
//    outBuffer_.Clear();
}


unsigned __stdcall PcqngRng::Runner(void* self)
{
    if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0)
        throw "PcqngCore: SetThreadAffinityMask FAILED";
    Sleep(1);

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
        throw "PcqngCore: SetThreadPriority FAILED";

    int phase = 0;
    int procCounter = 0;
    PcqngRng* obj = (PcqngRng*)self;
    uint64_t tscWord = 0;
    uint64_t prevTscWord = 0;
    int64_t tscDiff;
    LpFilter lpFilter;
    double qFactor;
    uint8_t target;

    uint8_t  eBits;
    uint8_t correctedWords[41];
    unsigned int makeTargetCount = 0;

    if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0)
        throw "PcqngCore: SetThreadAffinityMask FAILED";
    Sleep(1);

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
        throw "PcqngCore: SetThreadPriority FAILED";

//    static double bitcntt = 0;
    while (1)
    {
        if (WaitForSingleObject(obj->endEvent_, 0) == WAIT_OBJECT_0)
            break;


        // wait for multimedia timer fire
        DWORD waitResponse = WaitForSingleObject(obj->mmTimerEvent_, INFINITE);
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

        // target generation time
        if ((++makeTargetCount%8) == 0)
        {
            if(obj->targetBuffer->IsFull())
                continue;

            for (int b=0; b<7; b++)
                target = obj->targetCorrector.Next((eBits>>(6-b))&0x1);

            obj->targetBuffer->Write((char*)&target, 1);
        }
        else // regular generation
        {
            if (obj->rngBuffer == nullptr)
                continue;

            // corrector expansion
            for (int i=0; i<41; i++)
            {
                correctedWords[i] = 1;
                for (int b=0; b<7; b++)
                {
                    correctedWords[i] <<= 1;
                    correctedWords[i] |= obj->lfsrCorrector.Next((eBits>>(6-b))&0x1);
                }
            }
            obj->rngBuffer->Write((char*)correctedWords, 41);
        }
    }

    return 0;
}


int PcqngRng::Write(vector<uint8_t> writeBuffer)
{
    return 0;
}


int PcqngRng::SetBufferSizeMs(int bufferSizeMs)
{
    int newSize = (int)(GetBitRate()*bufferSizeMs/1000.0 + 0.5);
    rngBuffer->SetSize(newSize);

    return newSize;
}


int PcqngRng::GetBitRate()
{
    return 251125;
}


Buffer* PcqngRng::GetTargetBuffer()
{
    return targetBuffer;
}