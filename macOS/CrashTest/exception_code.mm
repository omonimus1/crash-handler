#import <Foundation/Foundation.h>
#import <dlfcn.h>
void handleException(NSException *exception) {
    // This line defines a function named 'handleException' that takes an NSException pointer as input.
    // This function will be called whenever an uncaught exception occurs in the application.

    NSLog(@"🚨 Uncaught Exception Detected! 🚨");
    // Prints a general warning message to the console indicating that an exception has been caught.

    NSLog(@"💥 EXC details : %@", exception);
    // Prints the details of the exception object itself.  This will often include
    // a description of the exception and potentially some other internal information.

    NSLog(@"Exception Name: %@", exception.name);
    // Prints the name of the exception (e.g., NSRangeException, NSInvalidArgumentException).
    // This helps identify the *type* of error.

    NSLog(@"Exception Reason: %@", exception.reason);
    // Prints the "reason" for the exception.  This is a human-readable string
    // explaining *why* the exception was thrown (e.g., "index out of bounds").

    NSArray *stackSymbols = [exception callStackSymbols];
    // Retrieves the call stack symbols at the time the exception was thrown.
    // The call stack is a list of function calls that led to the exception.
    // `stackSymbols` is an array of strings, where each string represents a frame
    // on the call stack.  These strings typically contain memory addresses and
    // (possibly) function names.

    NSLog(@"🔍 Full Stack Trace:");
    // Prints a header indicating that the full stack trace will be printed next.

    NSString *exceptionModulePath = nil;
    // Initializes a variable to store the path of the module (executable or library)
    // where the exception likely originated.  It's initially set to nil.

    // Iterate through the stack symbols in their original order
    for (NSString *symbol in stackSymbols) {
        // Starts a loop that iterates through each symbol (stack frame) in the `stackSymbols` array.

        NSLog(@"%@", symbol); // Print the raw symbol first
        // Prints the raw, unprocessed stack symbol string to the console.

        // More robust address extraction using regular expressions.
        NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"0x[0-9a-fA-F]+" options:0 error:nil];
        // Creates a regular expression object.  The pattern "0x[0-9a-fA-F]+"
        // matches a hexadecimal number (starting with "0x" followed by one or more
        // hexadecimal digits).  This is used to extract the memory address from the symbol string.

        NSTextCheckingResult *match = [regex firstMatchInString:symbol options:0 range:NSMakeRange(0, [symbol length])];
        // Attempts to find the first match of the regular expression within the current
        // stack symbol string. `match` will contain information about the match, or be nil if no match is found.

        if (match) {
            // Checks if a match was found (i.e., if a hexadecimal address was found in the symbol).

            NSString *addressString = [symbol substringWithRange:[match rangeAtIndex:0]];
            // Extracts the matched substring (the hexadecimal address) from the symbol.

            void *address = (void *)strtoul([addressString UTF8String], NULL, 16);
            // Converts the hexadecimal address string to an actual memory address (a void pointer).
            // `strtoul` is a standard C function for converting a string to an unsigned long.
            // The `16` specifies that the string is in base 16 (hexadecimal).

            Dl_info info;
            // Declares a `Dl_info` structure.  This structure will be used by `dladdr`
            // to store information about the dynamic library (or executable) containing the address.

            if (dladdr(address, &info)) {
                // Calls the `dladdr` function. This is the core of the module path lookup.
                // `dladdr` takes a memory address and a pointer to a `Dl_info` structure.
                // If it can find information about the address, it fills in the `Dl_info`
                // structure and returns a non-zero value (true).

                NSString *modulePath = [NSString stringWithUTF8String:info.dli_fname];
                // If `dladdr` was successful, this line extracts the path to the module
                // (the `dli_fname` field of the `Dl_info` structure) and converts it to an NSString.

                NSLog(@"  Module Path: %@", modulePath);
                // Prints the module path to the console.

                // Store the module path of the *first* frame that isn't a system library.
                // This is usually the best we can do to find the source of the crash.
                if (![modulePath containsString:@"/System/Library/"] &&
                    ![modulePath containsString:@"/usr/lib/"] &&
                    exceptionModulePath == nil)  // Only set it once
                {
                    // This is a crucial part of the logic. It checks if the module path:
                    // 1. Does *not* contain "/System/Library/" (a system library directory).
                    // 2. Does *not* contain "/usr/lib/" (another system library directory).
                    // 3. `exceptionModulePath` is still nil (meaning we haven't found a
                    //    non-system module yet).
                    // The goal is to find the *first* module in the call stack that is
                    // *not* a system library, as this is most likely the code that
                    // triggered the exception (your code, or a third-party library).

                    exceptionModulePath = modulePath;
                    // If all the conditions are met, the `exceptionModulePath` is set
                    // to the current `modulePath`.  This ensures that we only store the
                    // path of the *first* non-system module.
                }
            } else {
                NSLog(@"  dladdr failed for this address.");
                // If `dladdr` returns 0 (false), it means it couldn't find information
                // about the address.  This message is printed to the console.
            }
        } else {
            NSLog(@"  No address found in symbol.");
            // If the regular expression didn't find a hexadecimal address in the symbol,
            // this message is printed.
        }
    }

    if (exceptionModulePath) {
        // After iterating through all the stack symbols, this checks if
        // `exceptionModulePath` was ever set (i.e., if a non-system module was found).

        NSLog(@"🔍 Suspected module causing the exception: %@", exceptionModulePath);
        // If `exceptionModulePath` is not nil, it prints the likely module path.

    } else {
        NSLog(@"🔍 Could not determine the module path.");
        // If `exceptionModulePath` is still nil, it means no non-system module
        // was found in the call stack, and this message is printed.
    }


    exit(1); // Still exit after logging
    // Terminates the application with a non-zero exit code (1), indicating an error.
    // This ensures that the application doesn't continue running in a potentially
    // unstable state after an unhandled exception.
}
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Set the uncaught exception handler
        NSSetUncaughtExceptionHandler(&handleException);
        
        // Simulating a crash by accessing an out-of-bounds index
        NSArray *array = @[@"A", @"B", @"C"];
        NSLog(@"Accessing invalid index...");
        NSLog(@"%@", array[5]); // This will trigger an exception
        NSLog(@"Should never reach this line...");
    }
    return 0;
}
