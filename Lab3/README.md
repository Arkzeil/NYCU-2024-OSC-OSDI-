# Lab3
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab3.html)
---
## Basic Exercises
### Basic Exercise 1 - Exception
+ background
+ uart.c
    - Add 
+ cpio.c
    - Add a function that can give me address 
+ Debug using QEMU and gdb [ref1](https://henrybear327.gitbooks.io/gitbook_tutorial/content/Linux/GDB/index.html) [ref2](https://stackoverflow.com/questions/5429137/how-to-print-register-values-in-gdb)
    - The compiler and linker must add ```-g``` option to open debugging symbol
    1. First use QEMU to open gdb server:
    ```qemu-system-aarch64 -M raspi3b -kernel kernel8.img -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb -serial null -serial stdio -S -s```
    2. Use gdb(open another terminal) to connect to it
        - ```path/to/aarch64-gdb/aarch64-linux-gnu-gdb```
        - ```file kernel8.elf``` to load symbol table
        - set break points, like ```main``` or ```cpio_find```
        - ```target remote :1234``` to connect gdb server
        - Use ```continue```(execute until meeting break points), ```next```(view function as one instruction) or ```step``` to move to desired debugging block
        - Use ```display variable``` to observe variable value.
        - Use ```info registers x0```(or ```i r x0```) to check register value(It's case sensitive, so you should use upper-case for special-purpose registers like SPSR_EL0)
    - In my case, I comment out ```eret``` to make my program can be debugged(Since the program I load is actually a text file for test, so it will hanged)
    - I also add a gdb_break() as break point after the inline assembly to make it more convenient for debugging(but it's actually not necessary if you're willing to use step or next until the inline asm block is met)
    - Then make sure the value of ```SPSR_EL1```, ```ELR_EL1``` and ```SP_EL0``` is correct
    
### Basic Exercise 2 - Interrupt 
+ Background

### Basic Exercise 3 - Rpi3’s Peripheral Interrupt
+ Background

---
## Advanced Exercises
### Advanced Exercise 1 - Timer Multiplexing
+ Background

### Advanced Exercise 2 - Concurrent I/O Devices Handling
