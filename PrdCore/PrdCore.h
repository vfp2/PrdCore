// © 2012 Psigenics
//
// PrdCore: DLL interface to PrdCore
//
// 2012.10.03    pawilber        Migrated to VC++ 2012


#ifdef PRDCORE_EXPORTS
#define PRDCORE_API extern "C" __declspec(dllexport)
#else
#define PRDCORE_API extern "C" __declspec(dllimport)
#endif


PRDCORE_API int Open(char* licenseId);
PRDCORE_API int Close(int prdCoreHandle);
PRDCORE_API int ListSources(int srcsBuffLen, char* sources);
PRDCORE_API int SetSource(int prdCoreHandle, char* source);
PRDCORE_API int GetSource(int prdCoreHandle, int srcBuffLen, char* source);
PRDCORE_API int SetPrdMode(int prdCoreHandle, int prdMode);
PRDCORE_API int GetPrdMode(int prdCoreHandle);
PRDCORE_API int SetPsiMode(int prdCoreHandle, int psiMode);
PRDCORE_API int GetPsiMode(int prdCoreHandle);
PRDCORE_API int GetPrdData(int prdCoreHandle, int count, double* psiData, double* targetData);
PRDCORE_API int GetRawData(int prdCoreHandle, int count, char* rawData);
PRDCORE_API int Write(int prdCoreHandle, int outBuffLen, char* outBuffer);
// PRDCORE_API int SetBufferLength(int bufferLengthMs);