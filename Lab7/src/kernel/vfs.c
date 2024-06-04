#include "kernel/vfs.h"

int register_filesystem(struct filesystem* fs) {
  // register the file system to the kernel.
  // you can also initialize memory pool of the file system here.
  for(int i = 0; i < MAX_FS; i++){
    if(!filesystems[i].name){
      filesystems[i].name = fs->name;
      filesystems[i].setup_mount = fs->setup_mount;
      return i;
    }
  }

  return -1;
}

struct filesystem* get_fs(const char* name){
  // get the file system by name.
  for(int i = 0; i < MAX_FS; i++){
    if(string_comp(filesystems[i].name, name) == 0)
      return &filesystems[i];
  }
  return 0;
}

int vfs_open(const char* pathname, int flags, struct file** target) {
  // 1. Lookup pathname
  struct vnode *node;

  if(vfs_lookup(pathname, &node) != 0 && (flags & O_CREAT)){
    // 3. Create a new file if O_CREAT is specified in flags and vnode not found
    int last_slash_index = 0;
    char dir_name[MAX_PATHNAME + 1];

    for(int i = 0; i < string_len(pathname); i++){
      if(pathname[i] == '/')
        last_slash_index = i;
    }

    string_copy(dir_name, pathname);
    dir_name[last_slash_index] = '\0';
    // To see if we can find the vnode of the component(dir_name)
    if(vfs_lookup(dir_name, &node) != 0){
      // lookup error code shows if file exist or not or other error occurs
      uart_puts("Open Error: Cannot find the vnode of the dir\n");
      return -1;
    }
    // create a new file with last component name provided
    node->v_ops->create(node, &node, pathname + last_slash_index + 1);
    *target = (struct file*)pool_alloc(sizeof(struct file));
    // assign the vnode to target
    node->f_ops->open(node, target);
    (*target)->flags = flags;
    return 0;
  }
  // 2. Create a new file handle for this vnode if found.
  else{
    *target = (struct file*)pool_alloc(sizeof(struct file));
    // assign the file handle to target
    node->f_ops->open(node, target);
    (*target)->flags = flags;
    return 0;
  }
  // lookup error code shows if file exist or not or other error occurs
  // 4. Return error code if fails
  return -1;
}

int vfs_close(struct file* file) {
  // 1. release the file handle
  // 2. Return error code if fails
  file->vnode->f_ops->close(file);
}

int vfs_write(struct file* file, const void* buf, my_uint64_t len) {
  // 1. write len byte from buf to the opened file.
  // 2. return written size or error code if an error occurs.
  return file->vnode->f_ops->write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, my_uint64_t len) {
  // 1. read min(len, readable size) byte to buf from the opened file.
  // 2. block if nothing to read for FIFO type
  // 2. return read size or error code if an error occurs.
  return file->vnode->f_ops->read(file, buf, len);
}

int vfs_mkdir(const char* pathname){
  struct vnode *node;
  // this will invoke memset to set all the memory to 0
  char dir_name[MAX_PATHNAME + 1] = {};
  char new_dir_name[MAX_PATHNAME + 1] = {};

  int last_slash_index = 0;
  
  for(int i = 0; i < string_len(pathname); i++){
    if(pathname[i] == '/')
      last_slash_index = i;
  }
  for(int i = 0; i < last_slash_index; i++)
    dir_name[i] = pathname[i];

  string_copy(new_dir_name, pathname + last_slash_index + 1);

  if(vfs_lookup(dir_name, &node) != 0){
    uart_puts("Mkdir Error: Cannot find the vnode of the dir\n");
    return -1;
  }

  node->v_ops->mkdir(node, &node, new_dir_name);
  return 0;
}

int vfs_mount(const char* target, const char* filesystem){
  struct filesystem* fs = get_fs(filesystem);
  struct vnode* node;

  if(fs == 0){
    uart_puts("Mounting Error: Cannot find the filesystem\n");
    return -1;
  }

  if(vfs_lookup(target, &node) != 0){
    uart_puts("Mounting Error: Cannot find the vnode of the target\n");
    return -1;
  }

  node->mount = (struct mount*)pool_alloc(sizeof(struct mount));
  fs->setup_mount(fs, node->mount);

  return 0;
}
// lookup a vnode by pathname
int vfs_lookup(const char* pathname, struct vnode** target){
  // 1. Split the pathname into components
  // 2. Starting from rootfs, recursively find vnode of each component
  // 3. Return error code if not found
  // 4. Return found vnode
  if(string_len(pathname) == 0){
    *target = rootfs->root;
    return 0;
  }

  struct vnode* dir_node = rootfs->root;
  char component_name[MAX_FILE_NAME_LEN + 1];
  int component_index = 0;
  // split the pathname into components
  // then find the vnode of each component
  for(int i = 1; i < string_len(pathname); i++){
    if(pathname[i] == '/'){
      component_name[component_index++] = '\0';
      // To see if we can find the vnode of the component
      // If not, return -1
      if(dir_node->v_ops->lookup(dir_node, &dir_node, component_name))
        return -1;
      
      while(dir_node->mount)
        dir_node = dir_node->mount->root;
      // Get next component name
      component_index = 0;
    }
    else
      component_name[component_index++] = pathname[i];
  }

  component_name[component_index++] = '\0';

  if(dir_node->v_ops->lookup(dir_node, &dir_node, component_name))
    return -1;

  while(dir_node->mount)
    dir_node = dir_node->mount->root;

  *target = dir_node;
  return 0;
}

void init_rootfs(void){
  int index = tmpfs_register();
  
  rootfs = (struct mount*)pool_alloc(sizeof(struct mount));
  filesystems[index].setup_mount(&filesystems[index], rootfs);

  vfs_mkdir("/dev");
  
}