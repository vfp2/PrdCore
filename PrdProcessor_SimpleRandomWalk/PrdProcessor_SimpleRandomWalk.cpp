#include "stdafx.h"
#include "PrdProcessor_SimpleRandomWalk.h"
#include "IPrdProcessor.h"
#include "SimpleRandomWalkProcessor.h"


PRDPROCESSOR_SIMPLERANDOMWALK_API void* CreatePrdProcessor()
{
    IPrdProcessor* simpleRandomWalkProcessor = new SimpleRandomWalkProcessor();

    return (void*)simpleRandomWalkProcessor;
}
