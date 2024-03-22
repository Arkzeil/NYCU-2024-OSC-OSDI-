# Lab3
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab3.html)
---
## Basic Exercises
### Basic Exercise 1 - Exception
+ background
+ From EL2 to EL1:
    + boot.S
        - I put it in front of any initialization start (but this should after the device tree address acquiration as it will change x0)
        - ```SPSR_EL2```[ref](https://developer.arm.com/documentation/ddi0601/2023-12/AArch64-Registers/SPSR-EL2--Saved-Program-Status-Register--EL2-)
            - bit 4: Execution state. 0b0 indicates AArch64 execution state.
            - bit 6: FIQ, bit7: IRQ, bit 8: Serror, bit 9: Debug exception. Setting 1 will mask them to disable interrupts
        - Not sure why set spsr_el2 to 0x3C5(0011 1100 0101) as the bit[3:0] get no ```0101``` sequence in the ARM documentation
        > the [3:2] should indicat the EL value that  ```eret``` would change to. [ref](https://stackoverflow.com/questions/45878436/how-to-eret-to-the-same-exception-level-in-armv8)
        - Regarding ```hcr```:
            > Hypervisior configuration register. bit[31] is set indicate that the Execution state for EL1 is AArch64
        - Regarding ```eret```:
            > Reconstruct the processor state from current exception level's spsr_elx register and branch to the address in elr_elx
+ EL1 to EL0:
    - shell.c
        1. Use ```void *cpio_find(char *input)``` to get desired file address and put it in ```ELR_EL1```.
        2. Use ```simple_malloc``` to get the address of a new stack of size 2048 and put it on ```SP_EL0```
        3. We're now in EL1, so we modifiy the value of ```SPSR_EL1``` to 0x3C0 such that loading it can lead us to EL0 with interrupt disabled.
        4. Use eret to jump to user code
+ EL0 to EL1:
    - boot.S
        - Add ```exception_handler``` block, and the order is of opposite compared to the picture website provided. e.g. the first four are belonged exception from current EL while usig SP_EL0, and the first one is synchronous interrupt.
        - Regarding ```vbar_el1```
            > Vector Base Address Register: Holds the vector base address for any exception that is taken to EL1.
        - Regarding ```svc```
            > SuperVisor Call (SVC) instruction, with that the user can trigger an exception. Like a bridge between kernel mode and user mode.(ioctl)
+ Save context:
    - Just use the codes TA provided to save registers into stack.
+ uart.c
    - Add 
+ cpio.c
    - Add a function ```void *cpio_find(char *input)``` that can give me address of specific file inside initramfs. 
+ exception_hdlr.c
    - Add a exception handler function ```void c_exception_handler()``` to print out ```spsr_el1```, ```elr_el1```, and ```esr_el1``` 
---
+ FIQ and IRQ
    > Rpi3 has two levels of interrupt controllers. The first level controller routes interrupt to each CPU core, so each CPU core can have its timer interrupt and send interrupt processor Interrupts between each other
    > The second level controller routes interrupt from peripherals such as UART and system timer, they are aggregated and sent to the first level interrupt controller as GPU IRQ. 
    - And IRQ should be first kind of interrupt as FIQ be second.
    - [Another reference](https://blog.csdn.net/kickxxx/article/details/51612488) (But it seemed to be aarch32 version)
+ Regarding PSTATE: [ref](https://wiki.csie.ncku.edu.tw/embedded/ARMv8#process-state-pstate)
+ Regarding SPSEL: [ref1](https://blog.csdn.net/weixin_42135087/article/details/118334907) [ref2](https://stackoverflow.com/questions/65059491/why-save-init-task-struct-address-to-sp-el0-in-arm64-boot-code-primary-switche)
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
> When an interrupt occurs:
> 1. The CPSR/SPSR and ELR corresponding to the current Exception level are automatically saved.
> 2. The processor switches to the handling of the interrupt or exception.
> 3. After handling the interrupt, the saved state is restored, and execution resumes from the point where it was interrupted.
+ Parts I don't quite understand
> In the basic part, you only need to enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.

- Regarding ```cntp_ctl_el0```:
    > Control register for the EL1 physical timer.
    - ```cntp_ctl_el1```:
        > secure physical timer, usually accessible at EL3 but configurably accessible at EL1 in Secure state.
- Regarding ```cntp_tval_el0```:
    > Holds the timer value for the EL1 physical timer.
- Regarding ```CNTFRQ_EL0```:
    > This register is provided so that software can discover the frequency of the system counter. 
**This three seemed to be less relevent to the Exception Level its name indicates, not sure if there's other explaination**
- Regarding ```CORE0_TIMER_IRQ_CTRL```(it's not a register name, but a address pointing to core timer status registers):
    > 

+ ```c_core_timer_handler``` will print the seconds after booting and set the next timeout to 2 seconds later.(But it's not used in final code as the advanced part require more complicated handler. If you wanna use it, go to ```c_general_irq_handler``` and modify ```if(cpu_irq_src & (0x1 << 1))``` block, comment out ```mmio_write``` and ```c_timer_handler``` than uncomment ```c_core_timer_handler();```)
    - Setting ```cntp_tval_el0``` to desired value(```cntfrq_el0 * time```) to triger interrupt later.
### Basic Exercise 3 - Rpi3’s Peripheral Interrupt
+ Background
+ Disable interrupt
    - There's no single CPSR in aarch64 like in aarch32, so we user ```daif``` to manipulate DAIF bits
---
## Advanced Exercises
### Advanced Exercise 1 - Timer Multiplexing
+ Background
+ If there's exception handler function that will leads to the infinite c_exception_handler() invoking, then it's probably due to synchronous interrupt(Something wrong in the exception handler).
+ Why does the data copy in ```add_timer``` will not work when assigning it to task_timer->data? while function parameter can work?
    - If the malloc of struct is earlier than string malloc, then the string is corrupted?  -> Something wrong with my simple_malloc?

### Advanced Exercise 2 - Concurrent I/O Devices Handling
+ The receive task needs to put into task queue in interrupt handler, but the interrupt handler will not execute the task immediately until ExecTask() is called, so another interrupt may will happen as the intrrupt is still on and the data is not take out from register.
+ Be caution about the priority as write_handler called in recv_handler must has lower priority than recv_handler itself.
+ Nested interrupt: remember that the task queue may not cast out currently executing task from the queue, and if an interupt occurred inside that task, which will lead to an infinite execution of first task. -> change the head of task queue before executing callback function.
