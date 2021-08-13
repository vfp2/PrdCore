// © 2013 Psigenics
//
// PrdFtdiPrdDevice: Interface of a single FTDI PRD device
//
// 2013.02.28    pawilber        Stop command sent to device for power down. DetermineRates added.
// 2013.01.15    pawilber        Re-engineering/renamed class - Buffer is no longer part of the the device, external reference
// 2013.01.14    pawilber        AC stream generation on non-PD device; Bias/AC pairing; Optimizations to buffer read sizes
// 2012.09.28    pawilber        Interface of a single FTDI PRD device


#include "stdafx.h"
#include "FtdiPrdDevice.h"

#include <string>
#include <process.h>
#include <math.h>
#include <queue>


#define FTDIB_DEVICE_LATENCY_MS 4
#define FTDIB_DEVICE_PACKET_USB_SIZE 64
#define FTDIB_DEVICE_TX_TIMEOUT 2000

#define BASE_READ_SIZE 62


FtdiPrdDevice::FtdiPrdDevice(string id, Buffer& sourceBuffer)
    : id(id)
    , runnerThread(nullptr)
    , ftdiEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    , endEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
    , ftHandle(nullptr)
    , sourceBuffer(sourceBuffer)
    , nativeChannelCount(0)
    , chan0Stream(0)
    , chan0StreamBitCount(0)
    , chan1Stream(0)
    , chan1StreamBitCount(0)
{ }


FtdiPrdDevice::~FtdiPrdDevice()
{
    Stop();
    CloseHandle(endEvent);
    endEvent = nullptr;
    CloseHandle(ftdiEvent);
    ftdiEvent = nullptr;
}


void FtdiPrdDevice::Start()
{
    // already open?
    if (ftHandle != nullptr)
        return;

    // search for an available device
    if (memcmp(GetId().c_str(), "QWR1", 4) == 0)
    {
        if (FT_OpenEx("ComScire QNG Device", FT_OPEN_BY_DESCRIPTION, &ftHandle) != FT_OK)
            throw "NO HARDWARE"; // did not find an available device
    }
    else
    {
        if (FT_OpenEx((PVOID)GetId().c_str(), FT_OPEN_BY_SERIAL_NUMBER, &ftHandle) != FT_OK)
            throw "NO HARDWARE"; // did not find an available device
    }

    FT_SetLatencyTimer(ftHandle, FTDIB_DEVICE_LATENCY_MS);
	FT_SetUSBParameters(ftHandle, FTDIB_DEVICE_PACKET_USB_SIZE, FTDIB_DEVICE_PACKET_USB_SIZE);
	FT_SetTimeouts(ftHandle, FTDIB_DEVICE_TX_TIMEOUT, FTDIB_DEVICE_TX_TIMEOUT);

	// Start the device
	DWORD bytesWritten = 0;
	if (memcmp(GetId().c_str(), "PD", 2) == 0)
    {
		char prdStartCommand[1] = {0x36};
		if (FT_Write(ftHandle, prdStartCommand, 1, &bytesWritten) != FT_OK)
		{
			Stop();
			throw "NO HARDWARE";
		}
		if (bytesWritten != 1)
		{
			Stop();
			throw "NO HARDWARE";
		}
	}
	else
    {
		char prdStartCommand[4] = {(char)0x01, (char)0x00, (char)0xf0, (char)0x96};
		if (FT_Write(ftHandle, prdStartCommand, 4, &bytesWritten) != FT_OK)
		{
			Stop();
			throw "NO HARDWARE";
		}
		if (bytesWritten != 4)
		{
			Stop();
			throw "NO HARDWARE";
		}
	}
	DWORD rxBytes, txBytes, eventStatus;
	if (FT_GetStatus(ftHandle, &rxBytes, &txBytes, &eventStatus) != FT_OK)
	{
		Stop();
		throw "NO HARDWARE";
	}

    if (FT_SetEventNotification(ftHandle, FT_EVENT_RXCHAR, ftdiEvent) != FT_OK)
    {
        Stop();
        throw "NO HARDWARE";
    }

    // set native channel count
    nativeChannelCount = 1;
    if (id.find("PD") == 0)
    {
        if (id.find("QK") == 4)
            nativeChannelCount = 2;
        else
            nativeChannelCount = 16;
    }

    // start the reader thread
    ResetEvent(endEvent);
    runnerThread = (HANDLE)_beginthreadex(NULL, 0, FtdiPrdDevice::PrdRunner, this, 0, NULL);
}


void FtdiPrdDevice::Stop()
{
    // end reader thread
    SetEvent(endEvent);
    WaitForSingleObject(runnerThread, INFINITE);
    CloseHandle(runnerThread);
    runnerThread = nullptr;

    // stop hardware
    DWORD bytesWritten = 0;
    if (memcmp(GetId().c_str(), "PD", 2) == 0)
    {
        char prdStopCommand[4] = {(char)0x99};
        FT_Write(ftHandle, prdStopCommand, 1, &bytesWritten);
    }
    else
    {
        char prdStopCommand[4] = {(char)0x00, (char)0x00, (char)0x0f, (char)0xe0};
        FT_Write(ftHandle, prdStopCommand, 4, &bytesWritten);
    }

    // close hardware
    FT_Close(ftHandle);
    ftHandle = nullptr;
}


unsigned __stdcall FtdiPrdDevice::PrdRunner(void* self)
{
    FtdiPrdDevice* obj = (FtdiPrdDevice*)self;

//    obj->DetermineRates();

    DWORD bytesReturned = 0;
    vector<uint8_t> dataBuffer(FTDIB_DEVICE_PACKET_USB_SIZE);

    DWORD rxBytes;
    DWORD txBytes;
    DWORD eventDword;

    int biasFifoLength = 0;
    int acFifoLength = 0;

    while (1)
    {
        if (WaitForSingleObject(obj->endEvent, 0) == WAIT_OBJECT_0)
            break;

        WaitForSingleObject(obj->ftdiEvent, FTDIB_DEVICE_TX_TIMEOUT);
        FT_GetStatus(obj->ftHandle, &rxBytes, &txBytes, &eventDword);

        if (rxBytes > 0)
        {
            dataBuffer.resize(rxBytes);
            FT_Read(obj->ftHandle, &dataBuffer[0], rxBytes, &bytesReturned);

            if (obj->nativeChannelCount == 2) // 2 channal PRD
                obj->Chan2ToChan16(dataBuffer);
            if (obj->nativeChannelCount == 1) // QNG, etc.
                obj->Chan1ToChan16(dataBuffer);

			obj->sourceBuffer.Write(dataBuffer);
        }
    }

    return 0;
}


vector<string> FtdiPrdDevice::ListSources()
{
    vector<string> sourceList;

    DWORD ftDevCount = 0;
    FT_ListDevices(&ftDevCount, NULL, FT_LIST_NUMBER_ONLY);
    if (ftDevCount > 0)
    {
        for (DWORD devIndex=0; devIndex<ftDevCount; ++devIndex)
        {
            char serialNum[18];
            PVOID pDevIndex = 0;
            memcpy(&pDevIndex, &devIndex, sizeof(DWORD));
            serialNum[0] = '\0';
            FT_ListDevices((PVOID)pDevIndex, (PVOID)serialNum, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
            if (serialNum[0] == '\0')
            {
                FT_HANDLE ftHandle;
                if (FT_OpenEx("ComScire QNG Device", FT_OPEN_BY_DESCRIPTION, &ftHandle) == FT_OK)
                    strcpy_s(serialNum, 18, "QWR10000");
                FT_Close(ftHandle);
            }
            if ((memcmp(serialNum, "QWP", 3) == 0) || (memcmp(serialNum, "QWR", 3) == 0))
            {
                serialNum[8] = '\0';
                sourceList.push_back(serialNum);
            }
            if ((memcmp(serialNum, "PD", 2) == 0) && (serialNum[4] == 'Q'))
            {
                bool isValid = true;
                for (int i=6; i<=8; ++i)
                {
                    if ((serialNum[i] < '0') || (serialNum[i] > '9'))
                        isValid = false;
                }
                if (isValid == true)
                {
                    serialNum[16] = '\0';
                    sourceList.push_back(serialNum);
                }
            }
        }
    }

    return sourceList;
}


int FtdiPrdDevice::SendCommands(vector<uint8_t> writeBuffer) {
	DWORD bytesWritten = 0;
	if (memcmp(GetId().c_str(), "PD", 2) == 0) {
        if (FT_Write(ftHandle, &writeBuffer[0], writeBuffer.size(), &bytesWritten) == FT_OK)
			return bytesWritten;
		else
			return -1;
	}
	else
		return 0;
}


double FtdiPrdDevice::GetRawDataRate()
{
    int singleChannelDataRate = 0;

    if (id.find("QWR1") == 0)
        singleChannelDataRate = 1000000;
    if (id.find("QWR2") == 0)
        singleChannelDataRate = 2000000;
    if (id.find("QWR3") == 0)
        singleChannelDataRate = 32000000;

    if (id.find("QWP4") == 0)
        singleChannelDataRate = 250000;

    if (id.find("PD")==0 && id.find("QK")==4)
    {
        singleChannelDataRate = (id[6]-'0') * 100000;
        singleChannelDataRate += (id[7]-'0') * 10000;
        singleChannelDataRate += (id[8]-'0') * 1000;

        // loss due to matching pipeline
        singleChannelDataRate = (int)(singleChannelDataRate*0.99932);
    }

    if (id.find("PD")==0 && id.find("QM")==4)
    {
        singleChannelDataRate = (id[6]-'0') * 1000000;
        singleChannelDataRate += (id[7]-'0') * 100000;
        singleChannelDataRate += (id[8]-'0') * 10000;

        // loss due to matching pipeline
        singleChannelDataRate = (int)(singleChannelDataRate*0.99932);
    }

    return 2 * singleChannelDataRate;
}


int FtdiPrdDevice::Chan2ToChan16(vector<uint8_t>& dataBuffer)
{
    vector<uint8_t> processBuffer(dataBuffer);
    dataBuffer.clear();

    for (size_t i=0; i<processBuffer.size(); i++)
    {
        if ((processBuffer[i]&0x80) == 0x00)
        {
            chan0Stream |= ((uint32_t)processBuffer[i]<<chan0StreamBitCount)&0x7f;
            chan0StreamBitCount += 7;

            while (chan0StreamBitCount >= 4)
            {
                dataBuffer.push_back(chan0Stream&0x0f);

                chan0Stream >>= 4;
                chan0StreamBitCount -= 4;
            }
        }
        else
        {
            chan1Stream |= ((uint32_t)processBuffer[i]<<chan1StreamBitCount)&0x7f;
            chan1StreamBitCount += 7;

            while (chan1StreamBitCount >= 4)
            {
                dataBuffer.push_back(0x80 | (chan1Stream&0x0f));

                chan1Stream >>= 4;
                chan1StreamBitCount -= 4;
            }
        }
    }

    return dataBuffer.size();
}


int FtdiPrdDevice::Chan1ToChan16(vector<uint8_t>& dataBuffer)
{
    vector<uint8_t> processBuffer(dataBuffer);
    dataBuffer.clear();

    for (size_t i=0; i<processBuffer.size(); i++)
    {
        chan0Stream |= ((uint32_t)processBuffer[i]<<1)&0x1ff;

        dataBuffer.push_back(0x80 | ((chan0Stream>>1)&0x0f));
        dataBuffer.push_back((~(chan0Stream>>1)^(chan0Stream))&0x0f);

        dataBuffer.push_back(0x80 | ((chan0Stream>>5)&0x0f));
        dataBuffer.push_back((~(chan0Stream>>5)^(chan0Stream>>4))&0x0f);

        chan0Stream >>= 8;
    }

    return dataBuffer.size();
}


string FtdiPrdDevice::GetId()
{
    return id;
}


/*void FtdiPrdDevice::DetermineRates()
{
    DWORD bytesReturned = 0;
    vector<uint8_t> inBuffer(FTDIB_DEVICE_PACKET_USB_SIZE);

    DWORD rxBytes;
    DWORD txBytes;
    DWORD eventDword;

    FT_STATUS ftStatus;

    // boost thread priority
    DWORD priorityClass = GetPriorityClass(GetCurrentProcess());
    DWORD threadPriority = GetThreadPriority(GetCurrentThread());
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // purge buffers completely
    do
    {
        ftStatus = FT_StopInTask(ftHandle);
    } while (ftStatus != FT_OK);
    FT_Purge(ftHandle, FT_PURGE_RX);
    do
    {
        ftStatus = FT_RestartInTask(ftHandle);
    } while (ftStatus != FT_OK);

    // initial boostrap read cycles
    for (int i=0; i<10; i++)
    {
        do
        {
            WaitForSingleObject(ftdiEvent, FTDIB_DEVICE_TX_TIMEOUT);
            FT_GetStatus(ftHandle, &rxBytes, &txBytes, &eventDword);
//            QueryPerformanceCounter(&hpcCountB);
        } while (rxBytes == 0);
        inBuffer.resize(rxBytes);   
        FT_Read(ftHandle, &inBuffer[0], rxBytes, &bytesReturned);
    }

/*    while (
        WaitForSingleObject(ftdiEvent, FTDIB_DEVICE_TX_TIMEOUT);
        FT_GetStatus(ftHandle, &rxBytes, &txBytes, &eventDword);
        if (rxBytes > 0)
        {
            inBuffer.resize(rxBytes);
            FT_Read(ftHandle, &inBuffer[0], rxBytes, &bytesReturned);
            {
            
    // restore executon priority
    SetPriorityClass(GetCurrentProcess(), priorityClass);
    SetThreadPriority(GetCurrentThread(), threadPriority);
}*/