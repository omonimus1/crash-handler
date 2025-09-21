// Set terminate handler, unhanded exception filter, pure handler filter - in progress
#include <iostream>
#include <exception>
#include <typeinfo>
#include <cstdlib>
#include <windows.h>

// unhandled exception filter, vector handler, out of proc, handler, pure call, set_terminate
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

// Our custom terminate handler
void CustomTerminateHandler() {
    std::wcout << L"terminate handler: called" << std::endl;

    // Try to get the current exception
    try {
        std::exception_ptr eptr = std::current_exception();
        if (eptr) {
            // We have an exception, try to rethrow it to get information
            try {
                std::rethrow_exception(eptr);
            }
            catch (const std::exception& e) {
                // Standard exception with what() method
                std::wcout << L"Terminate handler: Exception type: " << typeid(e).name() << std::endl;
                std::wcout << L"Terminate handler: Exception message: " << e.what() << std::endl;
            }
            catch (...) {
                // Unknown exception type
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

    // Call the previous terminate handler if it exists
    if (previousTerminateHandler) {
        previousTerminateHandler();
    }
    else {
        std::wcout << L"No previous terminate handler, calling abort()" << std::endl;
        std::abort();
    }
}

// Our custom pure call handler
void CustomPureCallHandler() {
    std::wcout << L"pureCallHandler catched" << std::endl;

    // Additional information can be logged here
    std::wcout << L"A pure virtual function was called!" << std::endl;

    // Call the previous pure call handler if it exists
    if (previousPureCallHandler) {
        previousPureCallHandler();
    }
    else {
        std::wcout << L"No previous pure call handler, calling abort()" << std::endl;
        std::abort();
    }
}

// Unhandled exception filter
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers) {
    std::wcout << L"Unhandled exception filter called" << std::endl;

    if (pExceptionPointers && pExceptionPointers->ExceptionRecord) {
        std::wcout << L"Exception code: 0x" << std::hex
            << pExceptionPointers->ExceptionRecord->ExceptionCode << std::endl;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

// Classes to demonstrate pure virtual function call
class AbstractBase {
public:
    AbstractBase() {
        // Calling virtual function in constructor - this will trigger pure call handler
        CallPureVirtual();
    }
    virtual ~AbstractBase() {
        // Calling virtual function in destructor - this can also trigger pure call handler
        CallPureVirtual();
    }
    virtual void PureVirtualFunction() = 0;

    void CallPureVirtual() {
        PureVirtualFunction(); // This will trigger the pure call handler
    }
};

class ProblematicClass : public AbstractBase {
public:
    ProblematicClass() : AbstractBase() {
        std::wcout << L"ProblematicClass constructor completed" << std::endl;
    }

    ~ProblematicClass() {
        std::wcout << L"ProblematicClass destructor called" << std::endl;
    }

    void PureVirtualFunction() override {
        std::wcout << L"ProblematicClass::PureVirtualFunction called" << std::endl;
    }
};

// Alternative approach: Force pure call using assembly or direct call
class ForceBase {
public:
    virtual void PureFunction() = 0;

    void ForcePureCall() {
        // Get the vtable and call the pure virtual function directly
        void** vtable = *reinterpret_cast<void***>(this);
        typedef void(*PureFuncPtr)(ForceBase*);
        PureFuncPtr pureFunc = reinterpret_cast<PureFuncPtr>(vtable[0]);
        if (pureFunc) {
            pureFunc(this); // This should trigger pure call handler
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
        // Method 1: Constructor/Destructor approach
        std::wcout << L"Method 1: Constructor approach..." << std::endl;
        ProblematicClass* obj = new ProblematicClass();
        delete obj; // Destructor may also trigger it

        // Method 2: Direct vtable manipulation
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
        // Parse command line argument
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

    // This code will likely never be reached due to handlers calling abort()
    std::wcout << L"Program completed normally (this should rarely be seen)" << std::endl;
    return 0;
}
