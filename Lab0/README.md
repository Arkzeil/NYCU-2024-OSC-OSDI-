# Lab0  
---  
## Some background
+ Cross Compiler  
    - Download cross-compiler(ARM) from apt (```sudo apt install gcc-aarch64-linux-gnu```)
+ Linker
    - TA provides a incomplete pieces of code
    ```
    SECTIONS
    {
        . = 0x80000;
        .text : { *(.text) }
    }
    ```
    - ```ld --verbose``` can be used to check the content of default liker script file of current system.  
+ QEMU
    - An emulator for cross-platform development(Although QEMU provides a machine option for rpi3, it doesn’t behave the same as a real rpi3).  
    - Install it from apt
---  
## From Source Code to Kernel Image
+ From Source Code to Object Files
    - Try to cross compile a simple assembly file(a.S) TA provided to generate an .o file.
    ```
    .section ".text"
    _start:
        wfe
        b _start
    ```
    - Compile it using ```aarch64-linux-gnu-gcc -c a.S```(In my environment, I only have ```aarch64-none-linux-gnu-gcc```. But according to [this](https://stackoverflow.com/questions/13797693/what-is-the-difference-between-arm-linux-gcc-and-arm-none-linux-gnueabi), they're the same)
+ From Object Files to ELF  
    - 
+ From ELF to Kernel Image  
+ Check on QEMU  
---  
