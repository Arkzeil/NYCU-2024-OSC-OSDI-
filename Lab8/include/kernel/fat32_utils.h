#ifndef FAT32_UTILS_H
#define FAT32_UTILS_H

#include "kernel/vfs.h"

int fat32_getname(struct vnode *vd, char **name);
long fat32_getsize(struct vnode *vd);
int fat32_isdir(struct vnode *vd);

int fat32_get_next_cluster(unsigned int lba, int cluster_number);


#endif