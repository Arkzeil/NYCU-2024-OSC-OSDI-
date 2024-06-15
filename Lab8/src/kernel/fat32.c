#include "kernel/fat32.h"
// store the mount points
struct list_head mounts;

struct file_operations fat32_f_ops = {fat32_write, fat32_read, fat32_open, fat32_close, fat32_lseek64, fat32_getsize};
struct vnode_operations fat32_v_ops = {fat32_lookup, fat32_create, fat32_mkdir};

int fat32_register(){
    struct filesystem fs;
    fs.name = "fat32";
    fs.setup_mount = fat32_setup_mount;
    return register_filesystem(&fs);
}

int fat32_setup_mount(struct filesystem *fs, struct mount *mount){
    struct vnode *vnode;
    struct vnode *root_vnode = mount->root;
    struct fat32_inode *fat32;
    char buf[BLOCK_SIZE];
    mbr_partition_t *part;
    unsigned int lba;
    fat32_info_t *info;
    fat32_mount_t *fat32_mount;
    // read the first block of the disk
    readblock(0, buf);

    // check boot signature
    if(buf[0x1FE] != 0x55 && buf[0x1FF] != 0xAA ){
        uart_puts("fat32_setup_mount: boot signature error\n");
        return -1;
    }
    // get the first partition table
    part = (mbr_partition_t*)(buf[0x1BE]);
    // check if it's a fat32 partition
    if(part->type != 0x0B && part->type != 0x0C){
        uart_puts("fat32_setup_mount: not a fat32 partition\n");
        return -1;
    }

    vnode = pool_alloc(sizeof(struct vnode));
    info = pool_alloc(sizeof(fat32_info_t));
    fat32 = pool_alloc(sizeof(struct fat32_inode));
    fat32_mount = pool_alloc(sizeof(fat32_mount_t));

    lba = part->lba;
    // read the first block of the partition
    readblock(lba, buf);

    memcpy(&info->bs, buf, sizeof(fat32_boot_sector_t));

    info->fat_lba = lba + info->bs.reserved_sectors;
    info->cluster_lba = info->fat_lba + info->bs.sectors_per_fat32 * info->bs.fat_count;
    
    // copy the root vnode to the new vnode
    vnode->mount = root_vnode->mount;
    vnode->v_ops = root_vnode->v_ops;
    vnode->f_ops = root_vnode->f_ops;
    vnode->internal = root_vnode->internal;
    vnode->parent = root_vnode->parent;
    // set the new fat32 inode
    fat32->vnode = vnode;
    fat32->type = dir_t;
    fat32->info = info;
    fat32->cluster_num = 2;
    // set the root vnode with the new fat32 inode, and set the mount point
    root_vnode->mount = mount;
    root_vnode->v_ops = &fat32_v_ops;
    root_vnode->f_ops = &fat32_f_ops;
    root_vnode->internal = fat32;

    lock();
    list_add(&fat32_mount->list, &mounts);
    unlock();

    fat32_mount = mount;

    return 0;
}

struct vnode* fat32_create_vnode(struct mount* mount, enum node_type type){
    struct vnode *node = (struct vnode*)pool_alloc(sizeof(struct vnode));
    struct fat32_inode *inode = (struct fat32_inode*)pool_alloc(sizeof(struct fat32_inode));
    node->mount = mount;
    node->v_ops = &fat32_v_ops;
    node->f_ops = &fat32_f_ops;
    node->internal = inode;
    inode->type = type;
    return node;
}

int fat32_write(struct file *file, const void *buf, my_uint64_t len){
    struct fat32_inode *inode = (struct fat32_inode*)file->vnode->internal;
}

int fat32_read(struct file *file, void *buf, my_uint64_t len){}

int fat32_open(struct vnode *file_node, struct file **target){
    (*target)->vnode = file_node;
    (*target)->f_pos = 0;
    (*target)->f_ops = file_node->f_ops;
    return 0;
}

int fat32_close(struct file *file){
    pool_free(file);
    return 0;
}
int fat32_lseek64(struct file *file, long offset, int whence){

}

my_uint64_t fat32_getsize(struct vnode *vd){}

dir_entry_t *fat32_lookup_dir(struct vnode *dir_node, const char *component_name, char *buf){
    struct fat32_inode *dir_inode = (struct fat32_inode*)dir_node->internal;
    fat32_info_t *info = dir_inode->info;
    unsigned int cluster_num = dir_inode->cluster_num;
    unsigned int lba = info->cluster_lba + (cluster_num - 2) * info->bs.sectors_per_cluster; // get the lba of the cluster(it starts from 2)
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned int i = 0;
    dir_entry_t *entry;
    char *name;

    // read the first block of the cluster
    readblock(lba, buf);
    // get the first entry
    entry = (dir_entry_t*)buf;
    // find the entry
    while(entry->name[0] != 0){
        if(entry->name[0] == 0xE5){
            entry++;
            continue;
        }
        if(entry->attr == 0x0F){
            dir_long_entry_t *long_entry = (dir_long_entry_t*)entry;
            if(long_entry->order & 0x40){
                offset = (long_entry->order & 0x3F) - 1;
                size = 0;
                name = long_entry->name1;
            }
            else if(offset == (long_entry->order & 0x3F)){
                name = long_entry->name1;
            }
            else{
                uart_puts("fat32_lookup_dir: long entry error\n");
                return 0;
            }
        }
        else{
            if(offset == 0){
                name = entry->name;
            }
            else{
                uart_puts("fat32_lookup_dir: short entry error\n");
                return 0;
            }
        }
        if(string_comp(name, component_name) == 0){
            return entry;
        }
        entry++;
    }
    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){

}

int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name){
    if(!fat32_isdir(dir_node)){
        uart_puts("fat32_create: not a directory\n");
        return -1;
    }

    if(!fat32_lookup(dir_node, target, component_name)){
        uart_puts("fat32_create: file exists\n");
        return -1;
    }

    *target = fat32_create_vnode(dir_node->mount, file_t);

    return 0;
}

int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name){
    if(!fat32_isdir(dir_node)){
        uart_puts("fat32_mkdir: not a directory\n");
        return -1;
    }

    if(!fat32_lookup(dir_node, target, component_name)){
        uart_puts("fat32_mkdir: directory exists\n");
        return -1;
    }

    *target = fat32_create_vnode(dir_node->mount, dir_t);

    return 0;
}