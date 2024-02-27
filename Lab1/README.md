# Lab1
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab1.html)
---
## Basic Exercises  
### Basic Exercise 1 - Basic Initialization
+ Linker Script
    - Add ```.text``` ```.rodata``` ```.data``` ```.bss``` section [ref1](https://blog.louie.lu/2016/11/06/10%E5%88%86%E9%90%98%E8%AE%80%E6%87%82-linker-scripts/) [ref2](https://yodalee.me/2015/04/2015_linkerscript/#provide)
        - Regarding ```.data``` and ```.rodata```: [ref](https://blog.csdn.net/qq_26626709/article/details/51887085)
        > The .text, .rodata, and .data sections contain kernel-compiled instructions, read-only data, and normal data
        - Regarding ```.bss (NOLOAD)```: [ref1](https://zhuanlan.zhihu.com/p/27585869) [ref2](https://stackoverflow.com/questions/57181652/understanding-linker-script-noload-sections-in-embedded-software)
        > Align the section so that it starts at an address that is a multiple of 8. If the section is not aligned, it would be more difficult to use the str instruction to store 0
        - Regarding ```COMMON```: [ref](http://swaywang.blogspot.com/2012/06/elfbss-sectioncommon-section.html)
    - Save bss size in byte for future clearing zero at .S file
+ .S file
    - 
### Basic Exercise 2 - Mini UART
[BCM2837 ARM Peripherals manual](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)
> Physical addresses range from 0x3F000000 to 0x3FFFFFFF for peripherals. The bus addresses for peripherals are set up to map onto the peripheral bus address range starting at 0x7E000000. Thus a peripheral advertised here at bus address 0x7Ennnnnn is available at physical address 0x3Fnnnnnn.
+ Background
> UART stands for Universal asynchronous receiver-transmitter. This device is capable of converting values stored in one of its memory mapped registers to a sequence of high and low voltages. This sequence is passed to your computer via the TTL-to-serial cable and is interpreted by your terminal emulator.
+ Testing
    - Use qemu for testing: ```qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio```
        - Since we're using UART1 for this exercise, we must provide ```-serial null -serial stdio```
        > NOTE: qemu does not redirect UART1 to terminal by default, only UART0, so you have to use ```-serial null -serial stdio```.
        - Remember to use a linux terminal for this instruction as the VScode teminal might give you "qemu-system-aarch64: symbol lookup error:"
        - if you insist on using VScode terminal, add ```sudo```.
+ Note
    - I notice that in the example it transfer an unsigned int into ```uart_putc```, it should be the same?
    - BCM2837 p.11 AUX_MU_IO_REG Register:
    > 31:8 Reserved, write zero, read as don’t car

### Basic Exercise 3 - Simple Shell

### Basic Exercise 4 - Mailbox
+ Background
> Mailboxes facilitate communication between the ARM and the VideoCore.
+ Revision number
    - As using QEMU, it's emulating RPI 3b, so the revision number will be ```0x00A02082```?
+ Interesting(?) problem encountered
    - I got no output(stuck as soon I start QEMU) when using mailbox. If I comment the global ```mailbox``` array variable declared in ```mailbox.c``` or make it not an array, shell can still work. In the end, I found that I used w1 in my ```boot.S``` instead of w2. This is because that the w1 will destroy x1(upper 32 bits->0, lower: load the content) [ref](https://medium.com/vswe/aarch64-instruction-set-architecture-19d2d68392b)
---
## Advanced Exercises
### Advanced Exercise 1 - Reboot