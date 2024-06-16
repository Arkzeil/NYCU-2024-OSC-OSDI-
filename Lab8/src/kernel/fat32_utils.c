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
    // Get the FAT table, the use index to access the cluster entry
    lba += cluster_number / CLUSTER_ENTTY_PER_BLOCK;
    index = cluster_number % CLUSTER_ENTTY_PER_BLOCK;
    read_block(lba, buf);

    entry = &((fat32_cluster_entry_t*)buf)[index];

    return entry->value;
}

int alloc_cluster(fat32_info_t *info, unsigned int prev_cluster_num){
    fat32_cluster_entry_t *entry;
    int fat32_lba = info->fat_lba;
    int cluster_number = 0;
    short found;
    char buf[BLOCK_SIZE];
    // Find the first empty cluster
    while(fat32_lba < info->cluster_lba){
        read_block(fat32_lba, buf);
        entry = (fat32_cluster_entry_t*)buf;

        for(int i = 0; i < CLUSTER_ENTTY_PER_BLOCK; i++){
            if(entry[i].value == 0){
                found = 1;
                break;
            }
            cluster_number++;
        }

        if(found){
            break;
        }

        fat32_lba++;
    }

    if(!found){
        uart_puts("alloc_cluster: No empty cluster\n");
        return -1;
    }

    if(found && prev_cluster_num != 0){
        unsigned target_lba = info->cluster_lba + prev_cluster_num / CLUSTER_ENTTY_PER_BLOCK;
        unsigned target_index = prev_cluster_num % CLUSTER_ENTTY_PER_BLOCK;
        read_block(target_lba, buf);
        entry = (fat32_cluster_entry_t*)buf;
        entry[target_index].value = cluster_number;
        write_block(target_lba, buf);
    }
    
    return cluster_number;
}