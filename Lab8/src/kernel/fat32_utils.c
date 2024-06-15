#include "kernel/fat32_utils.h"
#include "kernel/fat32.h"

int fat32_getname(struct vnode *vd, char **name){
    *name = ((struct fat32_inode*)(vd->internal))->name;
    return 0;
}

int fat32_getsize(struct vnode *vd){
    if(((struct fat32_inode*)(vd->internal))->type == dir_t){
        uart_puts("fat32_getsize: Cannot get size of a directory\n");
        return 0;
    }
    return ((struct fat32_inode*)(vd->internal))->file->size;
}

int fat32_isdir(struct vnode *vd){
    return ((struct fat32_inode*)(vd->internal))->type == dir_t;
}

int fat32_get_next_cluster(unsigned int lba, int cluster_number){
    if(cluster_number >= 0x0FFFFFF8){
        uart_puts("fat32_get_next_cluster: end of cluster chain\n");
        return cluster_number;
    }

    int index;
    fat32_cluster_entry_t *entry;
    unsigned char buf[BLOCK_SIZE];

    lba += cluster_number / CLUSTER_ENTTY_PER_BLOCK;
    index = cluster_number % CLUSTER_ENTTY_PER_BLOCK;
    read_block(lba, buf);

    entry = &((fat32_cluster_entry_t*)buf)[index];

    return entry->value;
}

int alloc_cluster(fat32_info_t *info, unsigned int cluster_num){
    
}