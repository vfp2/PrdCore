#include "PcqngCoreTest.h"
#include "SimpleWalkPrdTest.h"
#include "PrdCoreDllTest.h"
#include <random>


int successes;
int failures;


int main()
{
    std::mt19937 rrng;
    LARGE_INTEGER pStart, pEnd, pStamp;
    LARGE_INTEGER pFreq;
    QueryPerformanceFrequency(&pFreq);

    QueryPerformanceCounter(&pStart);
    for (int i=0; i<1000000; i++) {
        QueryPerformanceCounter(&pStamp);
        rrng.seed(pStamp.LowPart);
    }
    QueryPerformanceCounter(&pEnd);

        double timeElapsed = (double)(pEnd.QuadPart-pStart.QuadPart) / (double)pFreq.QuadPart;

    successes = 0;
    failures = 0;

    PrdCoreDllTest::Test();
    SimpleWalkPrdTest::Test();
    PcqngCoreTest::Test();

    int testCount = successes + failures;
    if (failures > 0)
        printf("\n\n************ FAILURES DETECTED\n");
    printf("\n\nTests: %i    Successes: %i    Failures: %i\n", testCount, successes, failures);

    return 0;
}
