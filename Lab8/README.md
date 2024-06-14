# Lab8
[class webpage](https://nycu-caslab.github.io/OSC2024/labs/lab8.html)
---
## Basic Exercises
> The MBR is not located in a partition; it is located at a first sector of the device (physical offset 0), preceding the first partition.
1. Use ```truncate``` to create a empty image file
2. use ```fdisk``` to create DOS partition table and add new primary partition then assign MBR to LBA flag
3. use ```losetup``` to attatch loop device with image file(```--partscan``` to force kernel to scan all partitions of the image and attatch them)
4. use ```mkfs.vfat``` to make first partition of the image file become FAT32
5. create a directory and mount the first partition onto it
6. copy bootloader.img and initramfs.cpio to the FAT32 partition
7. unmount it and remove the loop device. So the first partition is now FAT32 and contain bootloader and initramfs
### Basic Exercise 1 - Open and Read
+ background

### Basic Exercise 2 - Create and Write
+ Background
### Basic Exercise 3 - Multitask VFS
### Basic Exercise 4 - /initramfs
+ 
### Advanced Exercise 1 - Memory Cached SD Card
+ 

