CROSS_PLATFORM = aarch64-linux-gnu
CC = gcc
LINKER = linker
KERNEL = kernel8
CFLAGS =  -Wall -Werror

ASM = $(wildcard *.S)

all: $(ASM) 
	$(CROSS_PLATFORM)-$(CC) -c $<
	$(CROSS_PLATFORM)-ld -T $(LINKER).ld -o $(KERNEL).elf a.o
	$(CROSS_PLATFORM)-objcopy -O binary $(KERNEL).elf $(KERNEL).img

clean:
	rm $(KERNEL).elf $(KERNEL).img *.o

run:
