// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <windows.h>

extern "C" __declspec(dllexport) void CrashFunction() {
    // Cause an intentional access violation crash
#pragma warning(push)
#pragma warning(disable : 6011)  // Disable null pointer dereference warning
#ifndef __clang_analyzer__
    int* ptr = nullptr;
    *ptr = 42;
#endif
#pragma warning(pop)
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

