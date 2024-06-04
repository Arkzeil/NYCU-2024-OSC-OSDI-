#ifndef TMPFS_H
#define TMPFS_H

#include "kernel/vfs.h"
#include "kernel/type.h"
#include "kernel/utils.h"

#define MAX_FILE_NAME_LEN 15
#define MAX_DIR_ENTRY 16
#define MAX_FILE_SIZE 4096

struct tmpfs_inode {
    char name[MAX_FILE_NAME_LEN];
    //struct tmpfs_node* parent;
    struct vnode* entry[MAX_DIR_ENTRY];
    char *data;
    enum node_type type;
    my_uint64_t data_size;
};

struct file_operations tmpfs_f_ops = {tmpfs_write, tmpfs_read, tmpfs_open, tmpfs_close, tmpfs_getsize};
struct vnode_operations tmpfs_v_ops = {tmpfs_lookup,tmpfs_create,tmpfs_mkdir};

int tmpfs_register();
int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount);
// create a vnode for tmpfs
struct vnode* tmpfs_create_vnode(struct mount* mount, enum node_type type);

int tmpfs_write(struct file *file, const void *buf, size_t len);
int tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, struct file **target);
int tmpfs_close(struct file *file);
my_uint64_t tmpfs_getsize(struct vnode *vd);

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

#endif