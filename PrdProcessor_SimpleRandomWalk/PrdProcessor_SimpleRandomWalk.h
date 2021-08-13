

#ifdef PRDPROCESSOR_SIMPLERANDOMWALK_EXPORTS
#define PRDPROCESSOR_SIMPLERANDOMWALK_API extern "C" __declspec(dllexport)
#else
#define PRDPROCESSOR_SIMPLERANDOMWALK_API extern "C" __declspec(dllimport)
#endif


PRDPROCESSOR_SIMPLERANDOMWALK_API void* CreatePrdProcessor();
