#pragma once


#include <string>
#include <thread>
#include <memory>

#include "Buffer.h"
#include "FtdiArrayRng.h"
#include "PcqngRng.h"
#include "PrngRng.h"
#include "SimpleWalkPrd.h"

using namespace std;


class PrdCoreObject
{
public:
	PrdCoreObject(string source);
	~PrdCoreObject(void);

	// checks the validity of the license ID, returns 0 if valid
	static int CheckLicense(char* licenseId);

    static vector<string> ListSources();
    string GetId();
    int PrdCoreObject::GetRawData(int count, char* rawData);
    int PrdCoreObject::Write(vector<uint8_t> writeBuffer);
    void GenPrdResult(int count, vector<double>& psiData, vector<double>& targetData);
    int GetPsiMode();
    void SetPsiMode(int psiMode);

private:
    IRng* rng;
    Buffer* rngBuffer;
    PcqngRng* pcqng;
    Buffer* targetBuffer;

    IPrd* prd;
};

