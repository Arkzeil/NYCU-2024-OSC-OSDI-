# Lab2
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab2.html)
---
## Basic Exercises
### Basic Exercise 1 - UART Bootloader
+ background
> In Lab 1, you might experience the process of moving the SD card between your host and rpi3 very often during debugging. You can eliminate this by introducing another bootloader to load the kernel under debugging.
+ Build a new bootloader kernel
    - boot.S
        - Since GPU always start at 0x80000, in order to run our bootloader, we first need to move out code(bootloader) from 0x80000 to address where linker declared(0x60000)
        - The ```_start``` in assembly stands for the begining of assembly, yet when we load new code into ```_start``` it's actually loaded into address pointed by ```_start```. Which are not equlivent, so there should not be overlapping problem
        - According to [ref](https://github.com/bztsrc/raspi3-tutorial/blob/master/14_raspbootin64/start.S), this relocate will only run on BSP core, so there no need to stop other cores. And below is the explaintion of GPT:
        > 1. Execution on BSP Core: The code is designed to run only on the Boot Strap Processor (BSP) core. The BSP core is typically responsible for bootstrapping the system and initializing other cores in a multi-core system. Since this code is intended to run early in the boot process, it assumes that it will execute on the BSP core.
        > 2. No Need to Check CPU's ID: In a multi-core system, each CPU core typically has a unique identifier. Normally, code that needs to run only on a specific core would check the CPU's ID to determine if it's running on the correct core. However, due to the specific firmware change mentioned, there's no need to perform this check in this case. The assumption is that the code will always execute on the BSP core.
        > 3. Spin-Loop on Non-Relocated Address: The comment warns about the consequences of running a spin-loop on a non-relocated address. A spin-loop is a tight loop that continuously checks a condition until it becomes true. If such a loop were to execute on a non-relocated address, it could cause issues, possibly due to the lack of proper initialization or the presence of unexpected data at that address.
        - The rest are the same as Lab1
    - linker.ld
        - Since 0x80000 is for new kernel, our .text start at 0x60000
        - Record the bootloader size so we can load them to 0x60000 in ```boot.S```
    - bootloader_main.c
        - ```char *kernel_addr = (char *)0x80000``` used to store where to put our loaded kernel.
        > a char pointer has the same alignment requirement as a void pointer.
        - ```kernel_size``` used to receive kernel size(in bytes) in little endian form.
        - Incrementally put received kernel data into 0x80000
        - Jump to 0x80000 to load new kernel
    - upload.py
        - Calculate kernel size in bytes and send them through serial in little endian form.
        - Send kernel through serial in bytes
+ Testing
    1. ```qemu-system-aarch64 -M raspi3b -kernel bootloader.img -serial null -serial pty```
    2. run ```upload.py```
    3. ```screen /dev/pts/7 115200``` to check the output(may not be '7', qemu will tell you which number it is)
### Basic Exercise 2 - Initial Ramdisk 
### Basic Exercise 3 - Simple Allocator
---
## Advanced Exercises
### Advanced Exercise 1 - Bootloader Self Relocation
### Advanced Exercise 2 - Devicetree