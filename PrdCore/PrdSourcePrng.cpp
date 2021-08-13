#include "stdafx.h"
#include "PrdSourcePrng.h"
#include <process.h>


PrdSourcePrng::PrdSourcePrng()
    : runnerThread(0)
    , endEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
    , initCount(10)
    , prevBit(0)
    , sourceBuffer(1000, false)
{
    SetBufferSizeMs(200);
    Start();
}


PrdSourcePrng::~PrdSourcePrng()
{
    Stop();
    CloseHandle(endEvent);
    endEvent = 0;
}


void PrdSourcePrng::Start()
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
    runnerThread = (HANDLE)_beginthreadex(NULL, 0, PrdSourcePrng::Runner, this, 0, NULL);
}


void PrdSourcePrng::Stop()
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


unsigned __stdcall PrdSourcePrng::Runner(void* self)
{
    PrdSourcePrng* obj = (PrdSourcePrng*)self;

    obj->twister.seed(GetTickCount());
    uint8_t data[72];

    while (1)
    {
        if (WaitForSingleObject(obj->endEvent, 0) == WAIT_OBJECT_0)
            break;

        // wait for multimedia timer fire
        DWORD waitResponse = WaitForSingleObject(obj->mmTimerEvent, INFINITE);
        if (waitResponse == WAIT_OBJECT_0)
        {
            int d = 0;
            for (int i=0; i<9; i++) {
                uint32_t rand32 = obj->twister();
                for (int j=0; j<4; j++)
                {
                    uint8_t rand8 = rand32>>(8*j);
                    data[2*d] = 0;
                    data[2*d+1] = 0;
                    for (int k=0; k<7; k++)
                    {
                        uint8_t newBit = (rand8>>k)&0x1;
                        data[2*d] |= (newBit<<7);
                        if (newBit == obj->prevBit)
                            data[2*d+1] |= 0x80;
                        obj->prevBit = newBit;
                        data[2*d] <<= 1;
                        data[2*d+1] <<= 1;
                    }
                    data[2*d] |= 0x80;
                    d++;
                }
            }

            obj->sourceBuffer.Write((char*)data, 72);
        }
    }

    return 0;
}


double PrdSourcePrng::GetRawDataRate()
{
    return 2 * 252000;
}


vector<uint8_t> PrdSourcePrng::GetRawData(int byteCount)
{
    vector<uint8_t> rawData(byteCount);
    sourceBuffer.Read(rawData, byteCount);

    return rawData;
}


string PrdSourcePrng::GetId()
{
    return "PRNG";
}


int PrdSourcePrng::SetRawBufferSize(int requestedRawBufferSize)
{
    sourceBuffer.SetSize(requestedRawBufferSize);

    return requestedRawBufferSize;
}


double PrdSourcePrng::SetBufferSizeMs(double bufferSizeMs)
{
    int newSize = (int)(2*GetRawDataRate()*bufferSizeMs/7000.0 + 0.5);
    sourceBuffer.SetSize(newSize);

    return (newSize*7000.0/(2*GetRawDataRate()));
}


int PrdSourcePrng::SendCommands(vector<uint8_t> writeBuffer)
{
    return 0;
}
