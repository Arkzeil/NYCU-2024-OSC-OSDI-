#ifndef FAT32_UTILS_H
#define FAT32_UTILS_H

#include "kernel/vfs.h"

int fat32_getname(struct vnode *vd, char **name);
int fat32_getsize(struct vnode *vd);
int fat32_isdir(struct vnode *vd);

int fat32_get_next_cluster(unsigned int lba, int cluster_number);
int alloc_cluster(fat32_info_t *info, unsigned int cluster_num);

#endif