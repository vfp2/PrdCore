// © 2012 Psigenics
//
// FtdiRng: Interface of a single FTDI PRD device
//
// 2012.09.28    pawilber        Interface of a single FTDI PRD device


#include "FtdiRng.h"
#include "Buffer.h"

#include <string>
#include <process.h>
#include <math.h>

#import "QWQNG.dll" no_namespace named_guids

#define FTDIB_DEVICE_LATENCY_MS 4
#define FTDIB_DEVICE_PACKET_USB_SIZE 320
#define FTDIB_DEVICE_TX_TIMEOUT 2000


FtdiRng::FtdiRng(string id, Buffer* rngBuffer)
    : IRng(id, rngBuffer)
    , runnerThread(nullptr)
    , ftdiEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
    , endEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
    , ftHandle(nullptr)
{
    SetBufferSizeMs(200);	
}


FtdiRng::~FtdiRng()
{
    Stop();
    CloseHandle(endEvent);
    endEvent = nullptr;
    CloseHandle(ftdiEvent);
    ftdiEvent = nullptr;
}


void FtdiRng::Start()
{
    // already open?
    if (ftHandle != nullptr)
        return;

    // search for an available device
    if (memcmp(GetId().c_str(), "QWR4", 4) == 0)
    {
        // start the reader thread
        ResetEvent(endEvent);
        runnerThread = (HANDLE)_beginthreadex(NULL, 0, FtdiRng::RunnerQWR4, this, 0, NULL);
    } 
    else 
    {
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
		    char prdStartCommand[1] = {0x66};
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

        // start the reader thread
        ResetEvent(endEvent);
        runnerThread = (HANDLE)_beginthreadex(NULL, 0, FtdiRng::Runner, this, 0, NULL);
    }
}


void FtdiRng::Stop()
{
    // end reader thread
    SetEvent(endEvent);
    WaitForSingleObject(runnerThread, INFINITE);
    CloseHandle(runnerThread);
    runnerThread = nullptr;

    // close hardware
    FT_Close(ftHandle);
    ftHandle = nullptr;
}

unsigned __stdcall FtdiRng::RunnerQWR4(void* self)
{
    FtdiRng* obj = (FtdiRng*)self;

    //QWQNG init
    CoInitialize(NULL); 
    IQNGPtr qng;
    qng.CreateInstance(CLSID_QNG);

    int rds = 0;

    while (1)
    {
        if (WaitForSingleObject(obj->endEvent, 0) == WAIT_OBJECT_0)
            break;
        //cout << "\n\nRd " << ++rds << "\n";
        _variant_t randvt = qng->RandBytes[FTDIB_DEVICE_PACKET_USB_SIZE]; 	
        VARIANT randvariant = randvt.GetVARIANT(); 	
        int bytecount = randvariant.parray->rgsabound[0].cElements 		
            - randvariant.parray->rgsabound[0].lLbound;  	
        uint8_t* randbyte; 	
        SafeArrayAccessData(randvariant.parray, (void**)&randbyte); 

        vector<uint8_t> inBuffer(randbyte, randbyte + bytecount);

        if (bytecount > 0)
        {
            //cout << "\n\n" << inBuffer.size() << "\n";
            //for (int i=0; i < inBuffer.size(); i++) {
            //    printf("%X ", inBuffer.at(i)&0xff); 
            //}
            //cout << "\n\n" << inBuffer.size() << "\n";
            obj->rngBuffer->Write(inBuffer);
        }
    }

    return 0;
}

unsigned __stdcall FtdiRng::Runner(void* self)
{
    FtdiRng* obj = (FtdiRng*)self;

    DWORD bytesReturned = 0;
    vector<uint8_t> inBuffer(FTDIB_DEVICE_PACKET_USB_SIZE);

    DWORD rxBytes;
    DWORD txBytes;
    DWORD eventDword;

    while (1)
    {
        if (WaitForSingleObject(obj->endEvent, 0) == WAIT_OBJECT_0)
            break;

        WaitForSingleObject(obj->ftdiEvent, INFINITE);
        FT_GetStatus(obj->ftHandle, &rxBytes, &txBytes, &eventDword);
        if (rxBytes > 0)
        {
            inBuffer.resize(rxBytes);
            FT_Read(obj->ftHandle, &inBuffer[0], rxBytes, &bytesReturned);

			obj->rngBuffer->Write(inBuffer);
        }
    }

    return 0;
}


vector<string> FtdiRng::ListSources()
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


int FtdiRng::Write(vector<uint8_t> writeBuffer) {
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


int FtdiRng::GetBitRate()
{
    int bitRate = 0;

    if (id.find("QWR1") == 0)
        bitRate = 875000;
    if (id.find("QWR2") == 0)
        bitRate = 875000;
    if (id.find("QWR4") == 0)
        bitRate = 100000;
    if (id.find("QWP4") == 0)
        bitRate = 250000;
    if (id.find("PD")==0 && id.find("QK")==4)
    {
        bitRate = (id[6]-'0') * 100000;
        bitRate += (id[7]-'0') * 10000;
        bitRate += (id[8]-'0') * 1000;
    }
    if (id.find("PD")==0 && id.find("QM")==4)
    {
        bitRate = (id[6]-'0') * 1000000;
        bitRate += (id[7]-'0') * 100000;
        bitRate += (id[8]-'0') * 10000;
    }

    return bitRate;
}


int FtdiRng::SetBufferSizeMs(int bufferSizeMs)
{
    double secondsFraction = (double)bufferSizeMs/1000.0;
    int bufferSizeBytes = (int)ceil(secondsFraction * (double)GetBitRate() / 7.0);  // 7 bits per byte

    rngBuffer->SetSize(bufferSizeBytes);
    return bufferSizeBytes;
}
