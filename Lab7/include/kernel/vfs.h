#ifndef VFS_H
#define VFS_H

#include "kernel/type.h"
#include "kernel/utils.h"
#include "kernel/allocator.h"
#include "kernel/tmpfs.h"

#define MAX_PATHNAME 255
#define O_CREAT 00000100
#define MAX_FS 0x50

// dir_t = 0, file_t = 1
enum node_type{
    dir_t,
    file_t
};

struct vnode {
  struct mount* mount;              // which mounted fs(superblock)
  struct vnode_operations* v_ops;   // vnode operations
  struct file_operations* f_ops;    // open file operations
  void* internal;                   // point to fs's vnode or inode
};

// file handle
struct file {
  struct vnode* vnode;              // which vnode is opened
  my_uint64_t f_pos;                // RW position of this file handle
  struct file_operations* f_ops;
  int flags;
};

struct mount {
  struct vnode* root;
  struct filesystem* fs;
};

struct filesystem {
  const char* name;
  int (*setup_mount)(struct filesystem* fs, struct mount* mount);
};

struct file_operations {
  int (*write)(struct file* file, const void* buf, my_uint64_t len);
  int (*read)(struct file* file, void* buf, my_uint64_t len);
  int (*open)(struct vnode* file_node, struct file** target);
  int (*close)(struct file* file);
  long (*lseek64)(struct file* file, long offset, int whence);
};

struct vnode_operations {
  // lookup a vnode in dir_node
  int (*lookup)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  // create a vnode in dir_node
  int (*create)(struct vnode* dir_node, struct vnode** target,
                const char* component_name);
  // mkdir a vnode in dir_node
  int (*mkdir)(struct vnode* dir_node, struct vnode** target,
              const char* component_name);
};

struct mount* rootfs;
struct filesystem filesystems[MAX_FS];


int register_filesystem(struct filesystem* fs);
struct filesystem* get_fs(const char* name);
int vfs_open(const char* pathname, int flags, struct file** target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, my_uint64_t len);
int vfs_read(struct file* file, void* buf, my_uint64_t len);
// e.g. mkdir "/dev/framebuffer"
int vfs_mkdir(const char* pathname);
// e.g. mount "devfs" on "/dev"
int vfs_mount(const char* target, const char* filesystem);
int vfs_lookup(const char* pathname, struct vnode** target);
void init_rootfs(void);

#endif