#include "IPrd.h"


IPrd::IPrd(IRng* rng, PcqngRng* pcqng)
    : rng(rng), pcqng(pcqng), targetMode(1), isAcProcessing(false)
{ }


string IPrd::GetId()
{
    return rng->GetId();
}


Buffer* IPrd::GetRngBuffer()
{
    return rng->GetRngBuffer();
}


Buffer* IPrd::GetTargetBuffer()
{
    return pcqng->GetTargetBuffer();
}


int IPrd::GetBitRate()
{
    return rng->GetBitRate();
}


int IPrd::GetPsiMode()
{
    return targetMode;
}


void IPrd::SetPsiMode(int psiMode)
{
    targetMode = psiMode;
}


bool IPrd::IsAcProcessing()
{
    return isAcProcessing;
}


void IPrd::SetAcProcessing(bool doAcProcessing)
{
    isAcProcessing = doAcProcessing;
}