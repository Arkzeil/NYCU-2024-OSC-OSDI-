# Lab0  
---  
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

