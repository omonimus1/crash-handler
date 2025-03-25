#import <Foundation/Foundation.h>

// Define the uncaught exception handler
void UncaughtExceptionHandler(NSException* _Nonnull exc) {
    NSLog(@"💥 Uncaught Exception: %@", exc);
    NSLog(@"📌 Exception reason: %@", [exc reason]);
    NSLog(@"📌 Exception call stack: %@", [exc callStackSymbols]);
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Set the uncaught exception handler
        NSSetUncaughtExceptionHandler(&UncaughtExceptionHandler);

        NSLog(@"🛠️  Starting crash handler proof-of-concept...");

        // **Intentional crash**: Access an invalid memory address (Null pointer dereference)
        NSObject *obj = nil;
        NSLog(@"This will crash: %@", [obj description]); // Crash occurs here

        NSLog(@"This line will never be reached.");
    }
    return 0;
}
