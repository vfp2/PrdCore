#include "IRng.h"


IRng::IRng(string id, Buffer* rngBuffer)
    : id(id), rngBuffer(rngBuffer)
{ }


IRng::~IRng()
{ }


string IRng::GetId()
{
    return id;
}


Buffer* IRng::GetRngBuffer()
{
    return rngBuffer;
}
