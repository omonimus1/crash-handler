#include <iostream>
#include <exception>
#include <typeinfo>
#include <cstdlib>
#include <windows.h>
#include <dbghelp.h>  // For stack trace functionality

#pragma comment(lib, "dbghelp.lib")  // Link against dbghelp.lib for symbol resolution

// Forward declarations
void CustomTerminateHandler();
void CustomPureCallHandler();
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers);
void TriggerPureCallHandler();
void TriggerTerminateHandler();
void TriggerDirectTerminate();

// Global variables to store previous handlers
std::terminate_handler previousTerminateHandler = nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER previousFilter = nullptr;
_purecall_handler previousPureCallHandler = nullptr;

// Function to print stack trace with symbol information
void PrintStackTrace() {
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);  // Initialize symbol handler

    void* stack[100];
    unsigned short frames = CaptureStackBackTrace(0, 100, stack, NULL);  // Capture stack frames

    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    if (!symbol) {
        std::wcout << L"Failed to allocate memory for symbol info" << std::endl;
        return;
    }
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    std::wcout << L"Stack trace:" << std::endl;
    for (unsigned short i = 0; i < frames; i++) {
        if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol)) {
            std::wcout << L"Frame " << i << L": " << symbol->Name
                << L" - 0x" << std::hex << symbol->Address << std::endl;
        }
        else {
            std::wcout << L"Frame " << i << L": Unknown - 0x" << std::hex << stack[i] << std::endl;
        }
    }

    free(symbol);
    SymCleanup(process);  // Clean up symbol handler
}

// Custom terminate handler
void CustomTerminateHandler() {
    std::wcout << L"terminate handler: called" << std::endl;

    try {
        std::exception_ptr eptr = std::current_exception();
        if (eptr) {
            try {
                std::rethrow_exception(eptr);
            }
            catch (const std::exception& e) {
                std::wcout << L"Terminate handler: Exception type: " << typeid(e).name() << std::endl;
                std::wcout << L"Terminate handler: Exception message: " << e.what() << std::endl;
            }
            catch (...) {
                std::wcout << L"Terminate handler: Unknown exception type" << std::endl;
            }
        }
        else {
            std::wcout << L" Terminate handler: No current exception" << std::endl;
        }
    }
    catch (...) {
        std::wcout << L"Terminate handler: Error while trying to extract exception information" << std::endl;
    }

    // Print stack trace for debugging
    PrintStackTrace();

    // Call previous handler if it exists, otherwise exit gracefully
    if (previousTerminateHandler) {
        previousTerminateHandler();
    }
    else {
        std::wcout << L"No previous terminate handler, exiting with error code" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

// Custom pure call handler with debugging information
void CustomPureCallHandler() {
    std::wcout << L"pureCallHandler caught" << std::endl;
    std::wcout << L"A pure virtual function was called!" << std::endl;
    std::wcout << L"This typically occurs when a virtual function is called during base class construction/destruction" << std::endl;
    std::wcout << L"or when an abstract class is instantiated." << std::endl;

    // Print stack trace to identify the cause
    PrintStackTrace();

    // Call previous handler if it exists, otherwise exit gracefully
    if (previousPureCallHandler) {
        previousPureCallHandler();
    }
    else {
        std::wcout << L"No previous pure call handler, exiting with error code" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

// Unhandled exception filter
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers) {
    std::wcout << L"Unhandled exception filter called" << std::endl;

    if (pExceptionPointers && pExceptionPointers->ExceptionRecord) {
        std::wcout << L"Exception code: 0x" << std::hex
            << pExceptionPointers->ExceptionRecord->ExceptionCode << std::endl;
    }

    // Print stack trace for debugging
    PrintStackTrace();

    return EXCEPTION_EXECUTE_HANDLER;
}

// Classes to demonstrate pure virtual function call
class AbstractBase {
public:
    AbstractBase() {
        // Calling virtual function in constructor - this will trigger pure call handler
        std::wcout << L"AbstractBase constructor" << std::endl;
        CallPureVirtual();
    }
    virtual ~AbstractBase() {
        // Calling virtual function in destructor - this can also trigger pure call handler
        std::wcout << L"AbstractBase destructor" << std::endl;
        CallPureVirtual();
    }
    virtual void PureVirtualFunction() = 0;

    void CallPureVirtual() {
        PureVirtualFunction(); // This will trigger the pure call handler during construction/destruction
    }
};

class ProblematicClass : public AbstractBase {
public:
    ProblematicClass() {
        std::wcout << L"ProblematicClass constructor completed" << std::endl;
    }

    ~ProblematicClass() {
        std::wcout << L"ProblematicClass destructor called" << std::endl;
    }

    void PureVirtualFunction() override {
        std::wcout << L"ProblematicClass::PureVirtualFunction called" << std::endl;
    }
};

// Alternative approach: Force pure call using vtable manipulation (but safe for fully constructed objects)
class ForceBase {
public:
    virtual void PureFunction() = 0;

    void ForcePureCall() {
        // This won't trigger pure call for fully constructed derived objects
        void** vtable = *reinterpret_cast<void***>(this);
        typedef void(*PureFuncPtr)(ForceBase*);
        PureFuncPtr pureFunc = reinterpret_cast<PureFuncPtr>(vtable[0]);
        if (pureFunc) {
            pureFunc(this);
        }
    }
};

class ForceDerived : public ForceBase {
public:
    void PureFunction() override {
        std::wcout << L"ForceDerived::PureFunction called" << std::endl;
    }
};

// Function to trigger pure call handler
void TriggerPureCallHandler() {
    std::wcout << L"Triggering pure call handler..." << std::endl;

    try {
        // Method 1: Constructor/Destructor approach - triggers pure call during construction
        std::wcout << L"Method 1: Constructor approach..." << std::endl;
        ProblematicClass* obj = new ProblematicClass();
        // If we reach here, construction succeeded, but destructor will also trigger on delete
        delete obj;

        // Method 2: Direct vtable approach (safe for fully constructed objects)
        std::wcout << L"Method 2: Direct vtable approach..." << std::endl;
        ForceDerived forced;
        ForceBase* basePtr = &forced;
        basePtr->ForcePureCall(); // Calls overridden function, no pure call

    }
    catch (...) {
        std::wcout << L"Exception caught while triggering pure call handler" << std::endl;
    }

    std::wcout << L"Pure call handler trigger attempt completed" << std::endl;
}

// Function to trigger terminate handler via exception
void TriggerTerminateHandler() {
    std::wcout << L"Triggering terminate handler..." << std::endl;
    throw std::runtime_error("Unhandled exception to trigger terminate handler");
}

// Function to trigger terminate handler directly
void TriggerDirectTerminate() {
    std::wcout << L"Triggering terminate handler directly via std::terminate()..." << std::endl;
    std::terminate();
}

int main(int argc, char* argv[]) {
    // Register all exception handlers first
    std::wcout << L"Registering exception handlers..." << std::endl;

    // Set up the terminate handler
    previousTerminateHandler = std::set_terminate(CustomTerminateHandler);
    std::wcout << L"Terminate handler registered" << std::endl;

    // Set up the pure call handler
    previousPureCallHandler = _set_purecall_handler(CustomPureCallHandler);
    std::wcout << L"Pure call handler registered" << std::endl;

    // Set up the unhandled exception filter
    previousFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
    std::wcout << L"Unhandled exception filter registered" << std::endl;

    std::wcout << L"All exception handlers have been registered successfully!" << std::endl;
    std::wcout << L"==========================================" << std::endl;

    // Check if command line argument was provided
    int choice = 0;
    if (argc > 1) {
        choice = std::atoi(argv[1]);
    }

    // If no valid command line argument, show menu
    if (choice < 1 || choice > 3) {
        std::wcout << L"Select the type of exception to trigger:" << std::endl;
        std::wcout << L"1: Pure call handler" << std::endl;
        std::wcout << L"2: Terminate handler (via exception)" << std::endl;
        std::wcout << L"3: Terminate handler (direct)" << std::endl;
        std::wcout << L"Enter your choice (1, 2, or 3): ";

        std::wcin >> choice;
    }

    std::wcout << L"==========================================" << std::endl;

    switch (choice) {
    case 1:
        TriggerPureCallHandler();
        break;
    case 2:
        TriggerTerminateHandler();
        break;
    case 3:
        TriggerDirectTerminate();
        break;
    default:
        std::wcout << L"Invalid choice. Exiting..." << std::endl;
        return 1;
    }

    std::wcout << L"Program completed normally" << std::endl;
    return 0;
}
