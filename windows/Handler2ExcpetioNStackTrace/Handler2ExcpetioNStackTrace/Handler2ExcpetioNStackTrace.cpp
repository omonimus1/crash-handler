#include <iostream>
#include <iomanip>
#include <windows.h>
#include <filesystem>
#include <psapi.h>


LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    std::cerr << " ************* \n";
    std::cerr << "Thread ID exception: " << GetCurrentThreadId() << "\n";

    // Get module info for faulting address

    PVOID exceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;
    auto exceptionCode = exceptionInfo->ExceptionRecord->ExceptionCode;
    std::cout << "exceptionCode: " << exceptionCode << std::endl;

    auto lastThreadError = GetLastError();
    //std::cout << "lastThreadError: " << lastThreadError << std::endl;

    // Get the base address of the module that contains the faulting address
    HMODULE hModule = NULL;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)exceptionAddress, &hModule))
    {
        std::cout << "Base address of faulting module: " << hModule << std::endl;

        // Get module file name
        char moduleName[MAX_PATH];
        if (GetModuleFileNameA(hModule, moduleName, MAX_PATH))
        {
            std::cout << "Crash in module: " << moduleName << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to get module information!" << std::endl;
    }


    //std::cerr << moduleInfo << " Unhandled exception caught! Code: "
    //    << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << "\n";

    std::cerr << "Faulting Address: "
        << exceptionInfo->ExceptionRecord->ExceptionAddress << "\n";

    std::cerr << " ************* \n";

    //std::cerr << moduleInfo << " Unhandled exception caught! Code: "
    //    << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << "\n";

    //std::cerr << moduleInfo << " Unhandled exception caught! Code: "
    //    << std::hex << exceptionInfo->ExceptionRecord->ExceptionAddress << "\n";

    // Print stack trace
    void* stack[100];
    WORD frames = CaptureStackBackTrace(0, 100, stack, nullptr);



    return EXCEPTION_EXECUTE_HANDLER;
}

void crash() {
    throw std::runtime_error("Something went wrong!");
}

int main() {
    SetUnhandledExceptionFilter(ExceptionHandler);

    std::cerr << "Starting application...\n";
    //crash(); // Trigger exception
    // Load the third-party DLL dynamically
    HMODULE thirdPartyDll = LoadLibraryA("SomeThirdParty.dll");
    if (!thirdPartyDll) {
        std::cerr << "Failed to load SomeThirdParty.dll\n";
        return 1;
    }

    // Get the function pointer
    typedef void (*ThirdPartyFunc)();
    ThirdPartyFunc crashFunc = (ThirdPartyFunc)GetProcAddress(thirdPartyDll, "CrashFunction");

    if (crashFunc) {
        std::cout << "Calling third-party crash function...\n";
        crashFunc();  // This should cause an intentional crash
    }
    else {
        std::cerr << "Failed to get CrashFunction address. Error: " << GetLastError() << std::endl;
    }
    return 0;
}