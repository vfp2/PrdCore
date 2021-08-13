#define LICENSE_ID_    "pawilber@gmail.com:hmn:nleepshgf"


#ifdef _DEBUG
#define DLLPATH_ "../Debug"
#else
#define DLLPATH_ "../Release"
#endif

#include "stdint.h"
#include <math.h>
#include "stdio.h"

typedef int (*FnOpen)(char* inLicenseId);
typedef int (*FnClose)(int inPrdCoreHandle);
typedef int (*FnListSources)(char* outPrdSources);
typedef int (*FnSetSource)(int inPrdCoreHandle, char* inPrdSource);
typedef int (*FnListProcessors)(char* outPrdProcessorsInfo);
typedef int (*FnSetProcessor)(int inPrdCoreHandle, char* inPrdProcessorId, double inRequestedTrialLengthMs, double* outActualTrialLengthMs);
typedef int (*FnGenTrial)(int inPrdCoreHandle, double* outTrialResult);
typedef int (*FnGetClairvoyanceTarget)(int inPrdCoreHandle, double* outClairvoyanceTarget);
typedef int (*FnGenPrecognitionTarget)(int inPrdCoreHandle, double* outPrecognitionTarget);
typedef int (*FnGetRawData)(int inPrdCoreHandle, int inRequestedBytes, char* outRawData); 
typedef int (*FnSetMode)(int inPrdCoreHandle, char inModeWord);

typedef int (*FnGetSource)(int inPrdCoreHandle, char* outPrdSource);


#include <Windows.h>
#include <direct.h>


int main()
{
        _chdir(DLLPATH_);
        // Import PrdCore DLL
        HINSTANCE prdCore = LoadLibraryA("PrdCore.dll");
        FnOpen PrdCoreOpen = (FnOpen)GetProcAddress(prdCore, "Open");
        FnClose PrdCoreClose = (FnClose)GetProcAddress(prdCore, "Close");
        FnGenTrial PrdCoreGenTrial = (FnGenTrial)GetProcAddress(prdCore, "GenTrial");
        FnSetMode PrdCoreSetMode = (FnSetMode)GetProcAddress(prdCore, "SetMode");
        FnGetRawData PrdCoreGetRawData = (FnGetRawData)GetProcAddress(prdCore, "GetRawData");

        FnGetSource PrdCoreGetSource = (FnGetSource)GetProcAddress(prdCore, "GetSource");

        int prdCoreHandle = PrdCoreOpen(LICENSE_ID_);

        char* sourceId = new char[256];
        PrdCoreGetSource(prdCoreHandle, sourceId);

        Sleep(1000);
        uint8_t data[500000];
        double s = 0;
        double ss = 0;

        int acFromBiasCount = 0;
        int acDirectCnt = 0;
        char prevBit = 0;
        int bCount = 0;
        int aCount = 0;

        for (int i=0; i<10; i++) {
            printf("*");
            PrdCoreGetRawData(prdCoreHandle, 50000, (char*)data);
        }

        //        PrdCoreGetRawData(prdCoreHandle, 10000, (char*)data);
        LARGE_INTEGER pf, pcs, pce;
        QueryPerformanceFrequency(&pf);
        QueryPerformanceCounter(&pcs);

        SYSTEMTIME startTime, endTime;
        FILETIME fts, fte;
        ULARGE_INTEGER st, et;
        GetSystemTime(&startTime);
        SystemTimeToFileTime(&startTime, &fts);
        st.HighPart = fts.dwHighDateTime;
        st.LowPart = fts.dwLowDateTime;

        double cnt = 0;
/*        while (1) {
                PrdCoreGetRawData(prdCoreHandle, 50000, (char*)data);
                cnt += 7*50000;

//                QueryPerformanceCounter(&pce);
                GetSystemTime(&endTime);
                SystemTimeToFileTime(&endTime, &fte);
                et.HighPart = fte.dwHighDateTime;
                et.LowPart = fte.dwLowDateTime;
                double seconds = (et.QuadPart-st.QuadPart)/10000000.0;
                printf("%i   %1.1f\n", (int)seconds, cnt/seconds);
        }
*/

        int acByte = 0;
        double misCnt =0;
        double tCnt = 0;
        DWORD start = GetTickCount();
        while (1) {
                PrdCoreGetRawData(prdCoreHandle, 100000, (char*)data);

        for (int i=0; i<100000; i++)
        {
            if (data[i]&0x80)
            {
                for (int j=0; j<7; j++)
                {
                    uint8_t newBit = (data[i]>>j)&0x1;
                    uint8_t acBit = (newBit==prevBit)?1:0;
                    acFromBiasCount += acBit;
                    acByte >>= 1;
                    acByte |= acBit<<6;
                    prevBit = newBit;
                }
                bCount++;
            }
            else
            {
                for (int j=0; j<7; j++)
                {
                    uint8_t newBit = (data[i]>>j)&0x1;
                    acDirectCnt += newBit;
                    if (acByte != data[i])
                    {
                        for (int k=0; k<7; k++)
                            if (((acByte>>k)&0x1) != ((data[i]>>k)&0x1))
                                misCnt++;
                    }
                    tCnt += 7;
                }
                aCount++;
            }
        }
        double bScore = (2.0*acFromBiasCount-7*bCount) / sqrt((double)7*bCount);
        double aScore = (2.0*acDirectCnt - 7*aCount) / sqrt((double)7*aCount);

        DWORD end = GetTickCount();
        double seconds = (end-start) / 1000.0;
        printf("%i   %1.2f   %1.0f\n", bCount,  7.0*acFromBiasCount/*seconds*/, bScore);
        printf("%i   %1.2f   %1.0f\n\n", aCount, 7.0*acDirectCnt/*seconds*/, aScore);
        printf("%i   %i    %1.10f\n\n", (int)tCnt, (int)misCnt, misCnt/tCnt);
        }

        double trialResult, acResult;
        PrdCoreGenTrial(prdCoreHandle, &trialResult);
        PrdCoreClose(prdCoreHandle);
}
