#include "PrngRng.h"
#include <process.h>


PrngRng::PrngRng(string id, Buffer* rngBuffer)
    : IRng(id, rngBuffer)
    , runnerThread_(0)
    , endEvent_(CreateEvent(NULL, TRUE, FALSE, NULL))
    , initCount_(10)
{
    Start();
}


PrngRng::~PrngRng()
{
    Stop();
    CloseHandle(endEvent_);
    endEvent_ = 0;
}



void PrngRng::Start()
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
    runnerThread_ = (HANDLE)_beginthreadex(NULL, 0, PrngRng::Runner, this, 0, NULL);
}


void PrngRng::Stop()
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


void PrngRng::ClearBuffer()
{
//    inBuffer_.Clear();
//    outBuffer_.Clear();
}


unsigned __stdcall PrngRng::Runner(void* self)
{
    PrngRng* obj = (PrngRng*)self;

    obj->twister.seed(GetTickCount());
    uint32_t data[9];

    while (1)
    {
        if (WaitForSingleObject(obj->endEvent_, 0) == WAIT_OBJECT_0)
            break;

        if (obj->rngBuffer->IsFull())
            continue;

        // wait for multimedia timer fire
        DWORD waitResponse = WaitForSingleObject(obj->mmTimerEvent_, INFINITE);
        if (waitResponse == WAIT_OBJECT_0)
        {
            for (int i=0; i<9; i++) {
                data[i] = obj->twister();
                data[i] |= 0x80808080;
            }

            obj->rngBuffer->Write((char*)data, 36);
        }
    }

    return 0;
}


int PrngRng::Write(vector<uint8_t> writeBuffer)
{
    return 0;
}


int PrngRng::SetBufferSizeMs(int bufferSizeMs)
{
    int newSize = (int)(GetBitRate()*bufferSizeMs/1000.0 + 0.5);
    rngBuffer->SetSize(newSize);

    return newSize;
}


int PrngRng::GetBitRate()
{
    return 256000;
}
