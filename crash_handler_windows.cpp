#include <iostream>
#include <exception>
#include <typeinfo>
#include <cstdlib>
#include <windows.h>
#include <dbghelp.h>  // For stack trace functionality
#include <processthreadsapi.h>  // For SetThreadStackGuarantee
#include <new>  // For std::set_new_handler and std::bad_alloc

#pragma comment(lib, "dbghelp.lib")  // Link against dbghelp.lib for symbol resolution

// Forward declarations
void CustomTerminateHandler();
void CustomPureCallHandler();
void CustomInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);
void CustomNewHandler();
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers);
void TriggerPureCallHandler();
void TriggerTerminateHandler();
void TriggerDirectTerminate();
void TriggerInvalidParameterHandler();
void TriggerNewHandler();

// Global variables to store previous handlers
std::terminate_handler previousTerminateHandler = nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER previousFilter = nullptr;
_purecall_handler previousPureCallHandler = nullptr;
_invalid_parameter_handler previousInvalidParameterHandler = nullptr;
std::new_handler previousNewHandler = nullptr;

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

// Custom invalid parameter handler
void CustomInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) {
    std::wcout << L"Invalid parameter handler called!" << std::endl;
    std::wcout << L"Details:" << std::endl;
    if (expression) {
        std::wcout << L"  Expression: " << expression << std::endl;
    }
    else {
        std::wcout << L"  Expression: (unknown)" << std::endl;
    }
    if (function) {
        std::wcout << L"  Function: " << function << std::endl;
    }
    else {
        std::wcout << L"  Function: (unknown)" << std::endl;
    }
    if (file) {
        std::wcout << L"  File: " << file << std::endl;
    }
    else {
        std::wcout << L"  File: (unknown)" << std::endl;
    }
    std::wcout << L"  Line: " << line << std::endl;

    // Print stack trace for debugging
    PrintStackTrace();

    // Call previous handler if it exists, otherwise exit
    if (previousInvalidParameterHandler) {
        previousInvalidParameterHandler(expression, function, file, line, pReserved);
    }
    else {
        std::wcout << L"No previous invalid parameter handler, exiting with error code" << std::endl;
        std::exit(EXIT_FAILURE);
    }
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
            std::wcout << L"Terminate handler: No current exception" << std::endl;
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

// Custom new handler for out-of-memory situations
void CustomNewHandler() {
    std::wcout << L"New handler called: Out of memory!" << std::endl;
    std::wcout << L"This occurs when operator new fails to allocate memory." << std::endl;

    // Print stack trace to identify the allocation site that caused the failure
    PrintStackTrace();

    // Call previous handler if it exists, otherwise throw std::bad_alloc
    if (previousNewHandler) {
        previousNewHandler();
    }
    else {
        std::wcout << L"No previous new handler, throwing std::bad_alloc" << std::endl;
        throw std::bad_alloc{};
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
        std::wcout << L"AbstractBase constructor" << std::endl;
        CallPureVirtual();
    }
    virtual ~AbstractBase() {
        std::wcout << L"AbstractBase destructor" << std::endl;
        CallPureVirtual();
    }
    virtual void PureVirtualFunction() = 0;

    void CallPureVirtual() {
        PureVirtualFunction();
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

// Alternative approach: Force pure call using vtable manipulation
class ForceBase {
public:
    virtual void PureFunction() = 0;

    void ForcePureCall() {
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
        std::wcout << L"Method 1: Constructor approach..." << std::endl;
        ProblematicClass* obj = new ProblematicClass();
        delete obj;

        std::wcout << L"Method 2: Direct vtable approach..." << std::endl;
        ForceDerived forced;
        ForceBase* basePtr = &forced;
        basePtr->ForcePureCall();
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

// Function to trigger invalid parameter handler
void TriggerInvalidParameterHandler() {
    std::wcout << L"Triggering invalid parameter handler..." << std::endl;
    // Example: Passing nullptr to printf, which triggers invalid parameter
    printf(nullptr);
    std::wcout << L"Invalid parameter handler trigger attempt completed" << std::endl;
}

// Function to trigger new handler (out-of-memory)
void TriggerNewHandler() {
    std::wcout << L"Triggering new handler (out-of-memory)..." << std::endl;
    try {
        // Allocate large chunks of memory in a loop to exhaust available memory
        // Note: This may take time or not trigger on systems with large/virtual memory
        while (true) {
            new int[100000000];  // Attempt to allocate ~400 MB each time
        }
    }
    catch (const std::bad_alloc& e) {
        std::wcout << L"Caught std::bad_alloc: " << e.what() << std::endl;
    }
    catch (...) {
        std::wcout << L"Unexpected exception caught while triggering new handler" << std::endl;
    }
    std::wcout << L"New handler trigger attempt completed" << std::endl;
}

int main(int argc, char* argv[]) {
    // Set thread stack guarantee for exception handling
    ULONG stackSize = 65536; // 64 KB for exception handling
    std::wcout << L"Setting thread stack guarantee to " << stackSize << L" bytes..." << std::endl;
    if (SetThreadStackGuarantee(&stackSize)) {
        std::wcout << L"Thread stack guarantee set successfully. Previous size: "
            << stackSize << L" bytes" << std::endl;
    }
    else {
        std::wcout << L"Failed to set thread stack guarantee. Error code: "
            << GetLastError() << std::endl;
    }

    // Register all exception handlers
    std::wcout << L"Registering exception handlers..." << std::endl;

    // Set up the terminate handler
    previousTerminateHandler = std::set_terminate(CustomTerminateHandler);
    std::wcout << L"Terminate handler registered" << std::endl;

    // Set up the pure call handler
    previousPureCallHandler = _set_purecall_handler(CustomPureCallHandler);
    std::wcout << L"Pure call handler registered" << std::endl;

    // Set up the invalid parameter handler
    previousInvalidParameterHandler = _set_invalid_parameter_handler(CustomInvalidParameterHandler);
    std::wcout << L"Invalid parameter handler registered" << std::endl;

    // Set up the new handler
    previousNewHandler = std::set_new_handler(CustomNewHandler);
    std::wcout << L"New handler registered" << std::endl;

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
    if (choice < 1 || choice > 5) {
        std::wcout << L"Select the type of exception to trigger:" << std::endl;
        std::wcout << L"1: Pure call handler" << std::endl;
        std::wcout << L"2: Terminate handler (via exception)" << std::endl;
        std::wcout << L"3: Terminate handler (direct)" << std::endl;
        std::wcout << L"4: Invalid parameter handler" << std::endl;
        std::wcout << L"5: New handler (out-of-memory)" << std::endl;
        std::wcout << L"Enter your choice (1, 2, 3, 4, or 5): ";
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
    case 4:
        TriggerInvalidParameterHandler();
        break;
    case 5:
        TriggerNewHandler();
        break;
    default:
        std::wcout << L"Invalid choice. Exiting..." << std::endl;
        return 1;
    }

    std::wcout << L"Program completed normally" << std::endl;
    return 0;
}
