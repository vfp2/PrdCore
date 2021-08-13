#define LICENSE_ID_    "pawilber@gmail.com:hmn:nleepshgf"

#ifdef _DEBUG
#define PRD_CORE_PATH_ "../Debug/PrdCore.dll"
#else
#define PRD_CORE_PATH_ "../Release/PrdCore.dll"
#endif

typedef int (*FnPrdListSources)(int srcsBuffLen, char* sources);
typedef int (*FnPrdOpen)(char* licenseId);
typedef int (*FnPrdClose)(int prdHandle);
typedef int (*FnPrdSetSource)(int prdHandle, char* source);
typedef int (*FnGetRawData)(int prdHandle, int count, char* rawData);
typedef int (*FnWrite)(int prdHandle, int count, char* writeBuffer);
typedef int (*FnGetPrdData)(int prdCoreHandle, int count, double* psiData, double* targetData);

class PrdCoreDllTest
{
public:
    static void Test()
    {
        // Import PrdCore DLL
        HINSTANCE prdCore = LoadLibraryA(PRD_CORE_PATH_);
        FnPrdListSources PrdListSources = (FnPrdListSources)GetProcAddress(prdCore, "ListSources");
        FnPrdOpen PrdOpen = (FnPrdOpen)GetProcAddress(prdCore, "Open");
        FnPrdClose PrdClose = (FnPrdClose)GetProcAddress(prdCore, "Close");
        FnPrdSetSource PrdSetSource = (FnPrdSetSource)GetProcAddress(prdCore, "SetSource");
        FnGetRawData GetRawData = (FnGetRawData)GetProcAddress(prdCore, "GetRawData");
        FnWrite Write = (FnWrite)GetProcAddress(prdCore, "Write");
        FnGetPrdData GetPrdData = (FnGetPrdData)GetProcAddress(prdCore, "GetPrdData");

        // Pre-allocate sources buffer to receive string array
        int sourceNameLen = 18;
        int maxSourceCount = 20;
        char* sources = (char*)malloc(maxSourceCount * sourceNameLen);

        // Read and list all available sources
        int sourceCount = PrdListSources(sourceNameLen*maxSourceCount, sources);
    
        std::vector<std::string> sourceList;
        char* source = sources;
        do {
            sourceList.push_back(source);
            source += strlen(source) + 1;
        } while (strlen(source) > 0);


        int prdHandle = PrdOpen(LICENSE_ID_);

        // Pre-allocate receiving data arrays
        char rawData[1000];

        LARGE_INTEGER freq, startCount, endCount;
        QueryPerformanceFrequency(&freq);

        for (int h=0; h<2; h++)
        {
            for (unsigned i=0; i<sourceList.size(); i++)
            {
                PrdSetSource(prdHandle, (char*)sourceList[i].c_str());
                char writeBuffer = 0x36;
                Write(prdHandle, 1, &writeBuffer);
                double psiData[20];
                double targetData[10];
                GetPrdData(prdHandle, 10, psiData, targetData);

                int targetRate = RateFromSerial(sourceList[i]);
                int measuredRate = 0;

                // select source
                char* source = (char*)sourceList[i].c_str();
                PrdSetSource(prdHandle, source);

                // remove about 300ms worth of data
                int removeCount = (int)(0.3*targetRate/7000);
                for (int j=0; j<removeCount; j++)
                    GetRawData(prdHandle, 1000, rawData);

                // measure about 10 seconds worth of data
                QueryPerformanceCounter(&startCount);
                int measureCount = (int)(10.0*targetRate/7000);
                for (int j=0; j<measureCount; j++)
                    GetRawData(prdHandle, 1000, rawData);
                QueryPerformanceCounter(&endCount);

                double rate = (endCount.QuadPart-startCount.QuadPart)/(double)freq.QuadPart;
                measuredRate = (int)(measureCount*7000.0/(rate+0.5));

                int hs = 9;
            }
        }
        PrdClose(prdHandle);
    }

    static int RateFromSerial(std::string id)
    {
        int bitRate = 0;
        
        if (id.find("QWR1") == 0)
            bitRate = 875000;
        if (id.find("QWR2") == 0)
            bitRate = 875000;
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
        if (id.find("PCQNG") == 0)
            bitRate = 252000;
        if (id.find("PRNG") == 0)
            bitRate = 250000;

        // bitrate if array
        int arraySize = 1;
        if (id.find("x") == 9)
        {
            arraySize = (id.at(10)-48);
            bitRate *= arraySize;
        }

        return bitRate;
    }
};
