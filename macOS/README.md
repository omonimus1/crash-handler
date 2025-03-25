## POC (proof of concept) for MacOS crash handler in Objective-C
- How to run:
```
clang -fobjc-arc -framework Foundation exception_code.mm -o module2 -lstdc++
./module2
```
- Goals:
- [X] Print module name
- [] Print exception code
- [X] Print exception name
- [X] Print exception reason
- [X] Print exception call stack
- [X] Print exception call stack symbols


