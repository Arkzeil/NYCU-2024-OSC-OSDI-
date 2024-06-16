#ifndef FAT32_H
#define FAT32_H

#include "kernel/vfs.h"
#include "kernel/list.h"
#include "kernel/sdhost.h"
#include "kernel/fat32_utils.h"

// ref: https://hackmd.io/@qy8LSFGCTDuQxhmEoSZjKQ/B1GPtg3YO

#define BLOCK_SIZE 512
#define CLUSTER_ENTTY_PER_BLOCK (BLOCK_SIZE / sizeof(struct fat32_cluster_entry))
#define DIR_ENTRY_PER_BLOCK (BLOCK_SIZE / sizeof(struct dir_entry))

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20    // Typically set by the operating system as soon as the file is created or modified to mark the file as "dirty", and reset by backup software once the file has been backed up to indicate "pure" state.
#define ATTR_LONG_NAME  0x0F



// ref: https://en.wikipedia.org/wiki/Master_boot_record
// patition table entry in MBR
typedef struct partition{
    unsigned char status;           // 0x80: bootable, 0x00: not bootable
    unsigned char chs_start[3];     // Cylinder-head-sector address of the first block in the partition(head->sector->cylinder)
    unsigned char type;             // Type of partition
    unsigned char chs_end[3];       // Cylinder-head-sector address of the last block in the partition((head->sector->cylinder))
    unsigned int lba;               // Logical block address of the first block in the partition
    unsigned int size;              // Size of the partition in sectors
}__attribute__((packed)) mbr_partition_t;

// ref: https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BPB
// Boot Sector in FAT32
typedef struct boot_sector{
    unsigned char jmp[3];
    unsigned char oem[8];
    // DOS 2.0 BPB
    unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char fat_count;            // Number of File Allocation Tables. Almost always 2;
    unsigned short root_entry_count;    // 0 for FAT32. Maximum number of FAT12 or FAT16 root directory entries.
    unsigned short total_sectors_short; // Total logical sectors. 0 for FAT32. (If zero, use 4 byte value at offset 0x020)
    unsigned char media_type;           // 0xF8 for fixed disk(i.e., typically a partition on a hard disk)
    unsigned short sectors_per_fat;     // Sectors per FAT. 0 for FAT32. (If zero, use 4 byte value at offset 0x024)
    // DOS 3.31 BPB
    unsigned short sectors_per_track;   // Sectors per track of storage device
    unsigned short head_side_count;     // Number of heads or sides of storage device, Number of heads for disks with INT 13h CHS geometry,[4] e.g., 2 for a double sided floppy.
    unsigned int hidden_sectors;        // Count of hidden sectors preceding the partition that contains this FAT volume( This field should always be zero on media that are not partitioned)
    unsigned int total_sectors_long;    // Total logical sectors including hidden sectors. If greater than 65535, use 4 byte value at offset 0x028(Total logical sectors (if greater than 65535; otherwise, see offset 0x013). )
    // FAT32 Extended BPB
    unsigned int sectors_per_fat32;      // Sectors per FAT for FAT32
    unsigned short flags;                // Flags
    unsigned short version;              // Version
    unsigned int root_cluster;           // Cluster number of the first cluster of the root directory for FAT32. Usually 2.
    unsigned short fsinfo_sector;        // Sector number of FSINFO structure for FAT32
    unsigned short backup_boot_sector;   // Sector number of a copy of the boot record for FAT32
    unsigned char reserved[12];          // Reserved
    // Extended BPB
    unsigned char drive_number;          // Drive number
    unsigned char reserved1;             // Reserved1
    unsigned char boot_signature;        // Extended boot signature
    unsigned int volume_id;              // Volume ID
    unsigned char volume_label[11];      // Volume label
    unsigned char fs_type[8];            // File system type
    //char boot_code[420];               // Boot code
    unsigned short boot_sector_signature;// Boot sector signature 0x55 0xAA
}__attribute__((packed)) fat32_boot_sector_t;

typedef struct fat32_info{
    fat32_boot_sector_t bs;           // Boot sector
    unsigned int fat_lba;       // FAT region lba
    unsigned int cluster_lba;   // data region lba
}fat32_info_t;

// ref: https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#FAT32
// SFN
typedef struct dir_entry{
    unsigned char name[8];          // File name
    unsigned char ext[3];           // File extension
    unsigned char attr;             // File attributes
    unsigned char lcase;            // Case for base and extension
    unsigned char ctime_cs;         // Creation time, centiseconds (0-199)
    unsigned short ctime;           // Creation time
    unsigned short cdate;           // Creation date
    unsigned short adate;           // Last access date
    unsigned short starthi;         // Start cluster (Hight 16 bits)
    unsigned short time;            // Last modify time
    unsigned short date;            // Last modify date
    unsigned short startlow;        // Start cluster (Low 16 bits)
    unsigned int size;              // File size in bytes
}__attribute__((packed)) dir_entry_t;
// LFN
typedef struct dir_long_entry{
    unsigned char order;            // The order of this entry in the sequence of long dir entries
    unsigned char name1[10];        // Characters 1-5 of the long-name sub-component in this entry
    unsigned char attr;             // ATTR_LONG_NAME
    unsigned char type;             // 0
    unsigned char checksum;         // Checksum of the name in the short dir entry
    unsigned char name2[12];        // Characters 6-11 of the long-name sub-component in this entry
    unsigned short start_cluster;   // Must be 0
    unsigned char name3[4];        // Characters 12-13 of the long-name sub-component in this entry
}__attribute__((packed)) dir_long_entry_t;

typedef struct fat32_cluster_entry{
    union{
        unsigned int value;
        // for the first 4 bits, it's reserved(little-endian)
        struct {
            unsigned int idx: 28;
            unsigned int reserved: 4;
        };
    };
}fat32_cluster_entry_t;

typedef struct file_name{
    union {
        unsigned char full_name[256];
        struct{
            unsigned char name[13];
        } part[20];  
    };
}__attribute__((packed)) file_name_t;


typedef struct fat32_mount{
    struct list_head list;
    struct mount *mount;
}fat32_mount_t;
// a list to store the directory entries
typedef struct fat32_dir_list{
    struct list_head list;
}fat32_dir_list_t;
// a list to store the file sizes
typedef struct fat32_file_list{
    struct list_head list;
    unsigned int size;
}fat32_file_list_t;

struct fat32_inode{
    // make it a list
    struct list_head list;
    char *name;
    struct vnode *vnode;
    fat32_info_t *info;
    unsigned int cluster_num;
    enum node_type type;
    union{
        fat32_dir_list_t *dir;
        fat32_file_list_t *file;
    };
};

int fat32_register();
int fat32_setup_mount(struct filesystem *fs, struct mount *mount);

struct vnode* fat32_create_vnode(struct vnode* parent, const char *name, unsigned int type, unsigned int cluster_num, unsigned int size);

int fat32_write(struct file *file, const void *buf, my_uint64_t len);
int fat32_read(struct file *file, void *buf, my_uint64_t len);
int fat32_open(struct vnode *file_node, struct file **target);
int fat32_close(struct file *file);
int fat32_lseek64(struct file *file, long offset, int whence);

my_uint64_t fat32_getsize(struct vnode *vd);

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);


#endif