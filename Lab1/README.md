# Lab1
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab1.html)
---
## Basic Exercises  
### Basic Exercise 1 - Basic Initialization
+ Linker Script
    - Add ```.text``` ```.rodata``` ```.data``` ```.bss``` section [ref1](https://blog.louie.lu/2016/11/06/10%E5%88%86%E9%90%98%E8%AE%80%E6%87%82-linker-scripts/) [ref2](https://yodalee.me/2015/04/2015_linkerscript/#provide)
        - Regarding ```.data``` and ```.rodata```: [ref](https://blog.csdn.net/qq_26626709/article/details/51887085)
        - Regarding ```.bss (NOLOAD)```: [ref1](https://zhuanlan.zhihu.com/p/27585869) [ref2](https://stackoverflow.com/questions/57181652/understanding-linker-script-noload-sections-in-embedded-software)
        - Regarding ```COMMON```: [ref](http://swaywang.blogspot.com/2012/06/elfbss-sectioncommon-section.html)
    - Save bss size in byte for future clearing zero at .S file
+ .S file
    - 
### Basic Exercise 2 - Mini UART

### Basic Exercise 3 - Simple Shell

### Basic Exercise 4 - Mailbox
---
## Advanced Exercises
### Advanced Exercise 1 - Reboot