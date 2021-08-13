#include "PrdCoreObject.h"
#include <iostream>



PrdCoreObject::PrdCoreObject(string source)
{
    rngBuffer = new Buffer(1000, false);
    targetBuffer = new Buffer(1000, true);

    if (source == "PCQNG")
    {
        pcqng = new PcqngRng("PCQNG", rngBuffer, targetBuffer);
        rng = pcqng;
    }
    else if (source == "PRNG")
    {
        pcqng = new PcqngRng("PCQNG", nullptr, targetBuffer);
        rng = new PrngRng(source, rngBuffer);
    }
    else
    {
        pcqng = new PcqngRng("PCQNG", nullptr, targetBuffer);
        rng = new FtdiArrayRng(source, rngBuffer);
    }

    prd = new SimpleWalkPrd(rng, pcqng);
    
    size_t s = (size_t)((0.200 * (double)rng->GetBitRate()) / 7);
    rngBuffer->SetSize(s);
//    printf("Buffer Size: %i\n", s);
}


PrdCoreObject::~PrdCoreObject(void)
{
    delete prd;
    if (rng != pcqng)
        delete rng;
    delete pcqng;
    delete targetBuffer;
    delete rngBuffer;
}


int PrdCoreObject::CheckLicense(char* licenseId)
{
    int idLen = (int)strlen(licenseId);

    int pwrdLen = 9;
    int tStampLen = 3;
    int userLen = idLen - pwrdLen - tStampLen - 2;
    if (userLen < 3)
        return (-1);

    char* userId = licenseId;
    char* tStamp = userId + userLen + 1;
    char* password = userId + userLen + tStampLen + 2;

    int userIndex = 0;
    // this is the dummy key
	char* key = "© 2013 Psigenics Corporation";
    // this is the real key
    key = "© 2007 Psigenics Corporation";

    userIndex = 0;
    for (int i=0; i<9; i++)
    {
        unsigned val = 0;
        if (i < 3)
            val = abs((int)((unsigned)password[i]-97) - (int)((unsigned)tStamp[i]%26) + 26)%26;
        else
        {
            val = abs((int)((unsigned)password[i]-97) - (int)((unsigned)userId[userIndex]%26) + 26)%26;
            userIndex++;
            userIndex %= userLen;
        }
        if (val != (unsigned)key[i]%26)
        {
			this_thread::sleep_for(chrono::seconds(1));
            return (-1);
        }
    }

    if (tStamp[2] != 'z')
    {
        time_t timeTNow;
        time(&timeTNow);
        tm localTNow;
        localtime_s(&localTNow, &timeTNow);
        if (((localTNow.tm_year%10) >= (tStamp[0]-97)) && ((localTNow.tm_mon) >= (tStamp[1]-97)) && ((localTNow.tm_mday) > (tStamp[2]-97)))
        {
			this_thread::sleep_for(chrono::seconds(1));
            return (-1);
        }
    }

	return 0;
}


vector<string> PrdCoreObject::ListSources()
{
    vector<string> sourceList;
    
    sourceList = FtdiArrayRng::ListSources();
    sourceList.push_back("PCQNG");
    sourceList.push_back("PRNG");
//    sourceList.push_back("PRNG");

    return sourceList;
}


string PrdCoreObject::GetId()
{
    return rng->GetId();
}


int PrdCoreObject::GetRawData(int count, char* rawData)
{
    vector<uint8_t> buff;
    count = rngBuffer->Read(buff, count);
    memcpy(rawData, &buff[0], count);

    return count;
}


int PrdCoreObject::Write(vector<uint8_t> writeBuffer)
{
    int wrx =  rng->Write(writeBuffer);
    if (rng->GetId().find("PD")==0 && (writeBuffer[0]&0xF0)==0x30)
        prd->SetAcProcessing(true);
    else
        prd->SetAcProcessing(false);

	return wrx;

}

void PrdCoreObject::GenPrdResult(int count, vector<double>& psiData, vector<double>& targetData)
{
    prd->GenPrdResult(count, psiData, targetData);
}


int PrdCoreObject::GetPsiMode()
{
    return prd->GetPsiMode();
}


void PrdCoreObject::SetPsiMode(int psiMode)
{
    prd->SetPsiMode(psiMode);
}
