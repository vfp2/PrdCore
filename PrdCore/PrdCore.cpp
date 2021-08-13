// © 2012 Psigenics
//
// PrdCore: DLL interface to PrdCore
//
// 2012.10.03    pawilber        Migrated to VC++ 2012


#include <Windows.h>
#include <time.h>
#include <iostream>

#include "PrdCore.h"
#include "PrdCoreObject.h"

//#include "Buffer.h"
//#include "PcqngCore.hpp"
//#include "PcqngRng.hpp"
//#include "FtdiArrayRng.hpp"
//#include "TwisterRng.hpp"
//#include "BpZAnalogFlipMultiPrd.hpp"
//#include "PrdComparator.hpp"
//#include "PcqngTargetGen.hpp"
//#include "PrdObject.h"


#include <map>
//#include <vector>

//#include <string>


using namespace std;


std::map<int, PrdCoreObject*> prdCoreHandles_;


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


PRDCORE_API int Open(char* licenseId)
{
    if (PrdCoreObject::CheckLicense(licenseId) == -1)
        return -1;

    // assign a unique prdHandle
    int prdHandle = 0;
    map<int, PrdCoreObject*>::iterator handleIter;
    do
    {
        prdHandle++;
        handleIter = prdCoreHandles_.find(prdHandle);

    } while (handleIter != prdCoreHandles_.end());

    prdCoreHandles_.insert(std::make_pair(prdHandle, new PrdCoreObject(PrdCoreObject::ListSources()[0])));
    
    return prdHandle;
}


PRDCORE_API int Close(int prdCoreHandle)
{
//    cout << "Close\n";
    // verify handle
    map<int, PrdCoreObject*>::iterator handleIter;
    handleIter = prdCoreHandles_.find(prdCoreHandle);
    if (handleIter == prdCoreHandles_.end())
        return -1;

    delete prdCoreHandles_[prdCoreHandle];
    prdCoreHandles_.erase(handleIter);

    return 0;
}


PRDCORE_API int ListSources(int srcsBuffLen, char* sources)
{
//    cout << "ListSources\n";

    vector<string> sourceList = PrdCoreObject::ListSources();

    // sufficient buffer size precondition check
    if (srcsBuffLen < (int)(17*sourceList.size()+1))
        return -1;

    unsigned stringPos = 0;
    for (unsigned i=0; i<sourceList.size(); i++)
    {
		strcpy_s(&sources[stringPos], sourceList[i].length()+1, sourceList[i].c_str());
        stringPos += (unsigned)sourceList[i].length() + 1;
    }

    // double null termination
    sources[stringPos] = '\0';

    return sourceList.size();
}


PRDCORE_API int SetSource(int prdCoreHandle, char* source)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    // do we already use this source?
    if (prdCoreObject->GetId().compare(source) == 0)
        return 0;

    // is this a valid (available?) source
    vector<string> sourceList = PrdCoreObject::ListSources();
    if (find(sourceList.begin(), sourceList.end(), string(source)) == sourceList.end())
        return -1;

   
    int psiMode = prdCoreObject->GetPsiMode();
    delete prdCoreObject;
    prdCoreHandles_.find(prdCoreHandle)->second = new PrdCoreObject(source);
    prdCoreObject = prdCoreHandles_[prdCoreHandle];
    prdCoreObject->SetPsiMode(psiMode);

    return 0;
}


PRDCORE_API int GetSource(int prdCoreHandle, int srcBuffLen, char* source)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    strcpy_s(source, srcBuffLen, prdCoreObject->GetId().c_str());

    return 0;
}


PRDCORE_API int SetPrdMode(int prdHandle, int prdMode)
{
    return 0;
}


PRDCORE_API int GetPrdMode(int prdHandle)
{
    return 0;
}


PRDCORE_API int SetPsiMode(int prdCoreHandle, int psiMode)
{
    if ((psiMode < 1) || (psiMode > 3))
        return -1;

    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    prdCoreObject->SetPsiMode(psiMode);

    return psiMode;
}


PRDCORE_API int GetPsiMode(int prdCoreHandle)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    return prdCoreObject->GetPsiMode();
}


PRDCORE_API int GetPrdData(int prdCoreHandle, int count, double* psiData, double* targetData)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    vector<double> vPsiData;
    vector<double> vTargetData;
    prdCoreObject->GenPrdResult(count, vPsiData, vTargetData);

    memcpy(psiData, &vPsiData[0], sizeof(double)*vPsiData.size());
    memcpy(targetData, &vTargetData[0], sizeof(double)*vTargetData.size());

    return 0;
}


PRDCORE_API int GetRawData(int prdCoreHandle, int count, char* rawData)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    return prdCoreObject->GetRawData(count, rawData);
}


PRDCORE_API int Write(int prdCoreHandle, int outBuffLen, char* outBuffer)
{
    PrdCoreObject* prdCoreObject = prdCoreHandles_[prdCoreHandle];

    vector<uint8_t> writeBuffer;
    for (int i=0; i<outBuffLen; i++)
        writeBuffer.push_back(outBuffer[i]);
    return prdCoreObject->Write(writeBuffer);
}
