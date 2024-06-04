#include "kernel/initramfs.h"

int initramfs_register(){
    struct filesystem fs;
    fs.name = "initramfs";
    fs.setup_mount = initramfs_setup_mount;
    // return the index of the filesystem
    return register_filesystem(&fs);
}

int initramfs_setup_mount(struct filesystem *fs, struct mount *mount){
    
}
// create a vnode for initramfs
struct vnode* initramfs_create_vnode(struct mount* mount, enum node_type type){
    struct vnode* vnode = (struct vnode*)pool_alloc(sizeof(struct vnode));
    struct initramfs_inode* inode = (struct initramfs_inode*)pool_alloc(sizeof(struct initramfs_inode));

    vnode->mount = mount;
    vnode->v_ops = &initramfs_v_ops;
    vnode->f_ops = &initramfs_f_ops;
    
    memset(inode, 0, sizeof(struct initramfs_inode));
    inode->type = type;
    inode->data = (char*)pool_alloc(MAX_FILE_SIZE);

    vnode->internal = inode;

    return vnode;
}

my_uint64_t initramfs_getsize(struct vnode *vd){
    struct initramfs_inode* inode = (struct initramfs_inode*)vd->internal;
    return inode->data_size;
}

int initramfs_write(struct file *file, const void *buf, size_t len){
    // it's read-only
    return -1;
}

int initramfs_read(struct file *file, void *buf, size_t len){}

int initramfs_open(struct vnode *file_node, struct file **target){}

int initramfs_close(struct file *file){
    pool_free(file);
    return 0;
}

int initramfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){}

int initramfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    // it's read-only
    return -1;
}
int initramfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    // it's read-only
    return -1;
}