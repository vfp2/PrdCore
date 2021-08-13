#pragma once

#ifdef PRDCORE_EXPORTS
#define PRDCORE_API extern "C" __declspec(dllexport)
#else
#define PRDCORE_API extern "C" __declspec(dllimport)
#endif


// core
PRDCORE_API int Open(char* inLicenseId);
PRDCORE_API int Close(int inPrdCoreHandle);
PRDCORE_API int GetSource(int inPrdCoreHandle, char* outPrdSource);
//PRDCORE_API int GenTrial(int inPrdCoreHandle, double* outTrialResult);

// control
//PRDCORE_API int ListSources(char* outPrdSources);
//PRDCORE_API int SetSource(int inPrdCoreHandle, char* inPrdSource);
//PRDCORE_API int GetSource(int inPrdCoreHandle, char* outPrdSource);
//PRDCORE_API int ListProcessors(char* outPrdProcessors);
//PRDCORE_API int SetProcessor(int inPrdCoreHandle, char* inPrdProcessor, int inMode, double inRequestedTrialLengthMs, double* outActualTrialLengthMs);
//PRDCORE_API int GetProcessor(int inPrdCoreHandle, char* outPrdProcessor, int* outMode, double* outTrialLengthMs);

// raw data api
PRDCORE_API int GetRawData(int inPrdCoreHandle, int inRequestedByteCount, char* outRawData);
PRDCORE_API int GetRawDataRate(int inPrdCoreHandle, double* outRawDataRate);
PRDCORE_API int SetRawBufferSize(int inPrdCoreHandle, int inRequestedRawBufferLength);
PRDCORE_API int GenRawTarget(int inPrdCoreHandle, char* outRawTarget);

//PRDCORE_API int GetClairvoyanceTarget(int inPrdCoreHandle, double* outClairvoyanceTarget);
//PRDCORE_API int GenPrecognitionTarget(int inPrdCoreHandle, double* outPrecognitionTarget);
//PRDCORE_API int SendCommands(int inPrdCoreHandle, int inCommandBufferByteCount, char* inCommandBuffer);