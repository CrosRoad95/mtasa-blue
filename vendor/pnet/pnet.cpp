//__declspec(dllexport)
#include "pnet.h"

HMODULE     hNetcModule;
BOOL WINAPI DllMain(HINSTANCE hinstDLL,             // handle to DLL module
                        DWORD     fdwReason,            // reason for calling function
                        LPVOID    lpvReserved)             // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            hNetcModule = LoadLibrary("D:\MTA SA Blue\Bin\mta\netc2.dll");
            // Initialize once for each new process.
            // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
            // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
            // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:

            if (lpvReserved != nullptr)
            {
                break;            // do not do cleanup if process termination scenario
            }

            // Perform any necessary cleanup.
            break;
    }
    return TRUE;            // Successful DLL_PROCESS_ATTACH.
}

// CheckCompatibility
// CheckService
// GetLibMtaVersion
// GetNetRel
// GetNetRev
// InitNetInterface
// InitNetRev
// NtTerminateProcess
// ReleaseNetInterface
// RoGetActivationFactory(x,x,x)
// 
// SetGTADirectory
// SetMTADirectory
// WaitForObject
// TlsCallback_0

__declspec(dllexport) unsigned long CheckCompatibility(unsigned long a, unsigned long* b)
{
    typedef unsigned long (*PFNCHECKCOMPATIBILITY)(unsigned long, unsigned long*);
    PFNCHECKCOMPATIBILITY fn = static_cast<PFNCHECKCOMPATIBILITY>((PVOID)GetProcAddress(hNetcModule, "CheckCompatibility"));
    return fn(a, b);
}

__declspec(dllexport) bool CheckService(unsigned int a)
{
    typedef bool (*PFNCheckService)(unsigned int);
    PFNCheckService fn = static_cast<PFNCheckService>((PVOID)GetProcAddress(hNetcModule, "CheckService"));
    return fn(a);
}

__declspec(dllexport) void GetLibMtaVersion(char* pBuffer, unsigned int uiMaxSize)
{
    typedef bool (*PFNGetLibMtaVersion)(char*, unsigned int);
    PFNGetLibMtaVersion fn = static_cast<PFNGetLibMtaVersion>((PVOID)GetProcAddress(hNetcModule, "GetLibMtaVersion"));
    fn(pBuffer, uiMaxSize);
}

__declspec(dllexport) unsigned short GetNetRel()
{
    typedef unsigned short (*PFNGetNetRel)();
    PFNGetNetRel fn = static_cast<PFNGetNetRel>((PVOID)GetProcAddress(hNetcModule, "GetNetRel"));
    return fn();
}

__declspec(dllexport) unsigned short GetNetRev()
{
    typedef short (*PFNGetNetRev)();
    PFNGetNetRev fn = static_cast<PFNGetNetRev>((PVOID)GetProcAddress(hNetcModule, "GetNetRev"));
    return fn();
}

__declspec(dllexport) void* InitNetInterface(void* a)
{
    typedef void* (*PFNInitNetInterface)(void*);
    PFNInitNetInterface fn = static_cast<PFNInitNetInterface>((PVOID)GetProcAddress(hNetcModule, "InitNetInterface"));
    return fn(a);
}

__declspec(dllexport) void InitNetRev(const char* a, const char* b, const char* c)
{
    typedef void (*PFNInitNetRev)(const char*, const char*, const char*);
    PFNInitNetRev fn = static_cast<PFNInitNetRev>((PVOID)GetProcAddress(hNetcModule, "InitNetRev"));
    fn(a, b, c);
}

__declspec(dllexport) int NtTerminateProcess(int a, unsigned int b)
{
    typedef int (*PFNNtTerminateProcess)(int, unsigned int);
    PFNNtTerminateProcess fn = static_cast<PFNNtTerminateProcess>((PVOID)GetProcAddress(hNetcModule, "NtTerminateProcess"));
    return fn(a, b);
}

__declspec(dllexport) int ReleaseNetInterface()
{
    typedef int (*PFNReleaseNetInterface)();
    PFNReleaseNetInterface fn = static_cast<PFNReleaseNetInterface>((PVOID)GetProcAddress(hNetcModule, "ReleaseNetInterface"));
    return fn();
}

__declspec(dllexport) int RoGetActivationFactory(int a, int b, int c)
{
    typedef int (*PFNRoGetActivationFactory)(int, int, int);
    PFNRoGetActivationFactory fn = static_cast<PFNRoGetActivationFactory>((PVOID)GetProcAddress(hNetcModule, "RoGetActivationFactory"));
    return fn(a, b, c);
}

__declspec(dllexport) void SetGTADirectory(const wchar_t* a, size_t b)
{
    typedef void (*PFNSetGTADirectory)(const wchar_t*, size_t);
    PFNSetGTADirectory fn = static_cast<PFNSetGTADirectory>((PVOID)GetProcAddress(hNetcModule, "SetGTADirectory"));
    fn(a, b);
}
//
//__declspec(dllexport) unsigned long WaitForObject(HANDLE hProcess, HANDLE hThread, DWORD dwMilliseconds, HANDLE hMutex)
//{
//    typedef unsigned long (*PFNWaitForObject)(HANDLE, HANDLE, DWORD, HANDLE);
//    PFNWaitForObject fn = static_cast<PFNWaitForObject>((PVOID)GetProcAddress(hNetcModule, "WaitForObject"));
//    return fn(hProcess, hThread, dwMilliseconds, hMutex);
//}




