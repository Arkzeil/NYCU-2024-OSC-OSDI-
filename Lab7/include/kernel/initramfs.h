#ifndef INITRAMFS_H
#define INITRAMFS_H

#include "kernel/vfs.h"

struct initramfs_inode{
    char *name;                             // no length specified in doc
    struct vnode *entry[MAX_DIR_ENTRY];
    char *data;
    enum node_type type;
    my_uint64_t data_size;
};

struct file_operations initramfs_f_ops = {initramfs_write, initramfs_read, initramfs_open, initramfs_close, initramfs_getsize};
struct vnode_operations initramfs_v_ops = {initramfs_lookup, initramfs_create, initramfs_mkdir};

int initramfs_register();
int initramfs_setup_mount(struct filesystem *fs, struct mount *mount);
// create a vnode for initramfs
struct vnode* initramfs_create_vnode(struct mount* mount, enum node_type type);

int initramfs_write(struct file *file, const void *buf, size_t len);
int initramfs_read(struct file *file, void *buf, size_t len);
int initramfs_open(struct vnode *file_node, struct file **target);
int initramfs_close(struct file *file);
my_uint64_t initramfs_getsize(struct vnode *vd);

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

#endif