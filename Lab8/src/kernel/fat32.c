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

struct vnode* fat32_create_vnode(struct vnode* parent, const char *name, unsigned int type, unsigned int cluster_num, unsigned int size){
    struct vnode *node = (struct vnode*)pool_alloc(sizeof(struct vnode));
    struct fat32_inode *parent_inode = parent->internal;
    struct fat32_inode *inode = (struct fat32_inode*)pool_alloc(sizeof(struct fat32_inode));
    char buf = pool_alloc(string_len(name) + 1);

    string_copy(buf, name);

    inode->name = buf;
    inode->vnode = node;
    inode->info = parent_inode->info;
    inode->cluster_num = cluster_num;
    inode->type = type;

    if(type == dir_t){
        fat32_dir_list_t *dir = pool_alloc(sizeof(fat32_dir_list_t));
        INIT_LIST_HEAD(&dir->list);
        inode->dir = dir;
    }
    else{
        fat32_file_list_t *file = pool_alloc(sizeof(fat32_file_list_t));
        INIT_LIST_HEAD(&file->list);
        file->size = size;
        inode->file = file;
    }

    node->mount = parent->mount;
    node->v_ops = &fat32_v_ops;
    node->f_ops = &fat32_f_ops;
    node->internal = inode;
    node->parent = parent;
    // attach to parent's dir_t or file_t list
    list_add(&inode->list, &parent_inode->dir->list);

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
    unsigned int lba;
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned int i = 0;
    dir_entry_t *entry;
    file_name_t name;
    short found = 0;
    short dir_end = 0;
    short LFN = 0;

    while(1){
        lba = info->cluster_lba + (cluster_num - 2) * info->bs.sectors_per_cluster; // get the lba of the cluster(it starts from 2)
        // read the first block of the cluster
        readblock(lba, buf);
        // find the entry
        for(int i = 0; i < DIR_ENTRY_PER_BLOCK; i++){
            // get the i_th entry
            entry = (dir_entry_t*)(&buf[sizeof(dir_entry_t) * i]);

            if(entry->name[0] == 0x0){
                dir_end = 1;
                break;
            }
            // check if it's a long entry
            if((entry->attr & 0x0F) == 0x0F){
                // Get the sequence number of LFN by accessing first byte of SFN
                // the sequence number is the first 6 bits of the first byte
                // and the sequence number starts from 1
                int n = (entry->name[0] & 0x3F) - 1;
                LFN = 1;
                dir_long_entry_t *long_entry = (dir_long_entry_t*)entry;
                // Since a UCS-2(unicode) is 2 bytes(chars), we seemed only need the first byte(little endian)?
                // so we add index by 2 and divide the index by 2
                for(int j = 0; long_entry->name1[j] != 0xFF && j < 10; j+=2)
                    name.part[n].name[j / 2] = long_entry->name1[j];

                for(int j = 0; long_entry->name2[j] != 0xFF && j < 12; j+=2)
                    name.part[n].name[5 + j / 2] = long_entry->name2[j];
                
                for(int j = 0; long_entry->name3[j] != 0xFF && j < 4; j+=2)
                    name.part[n].name[11 + j / 2] = long_entry->name3[j];

                continue;
            }
            if(LFN){
                if(string_icase_comp(name.full_name, component_name) == 0){
                    found = 1;
                    break;
                }
                LFN = 0;
                memset(&name, 0, sizeof(file_name_t));
                continue;
            }

            LFN = 0;
            // if it's SFN, we need to check the filename and extension
            unsigned int len = 8;
            // find the length of the filename by checking the space
            while(len){
                if(entry->name[len - 1] == ' ')
                    len--;
                else
                    break;
            }
            
            memcpy(name.full_name, entry->name, len);

            len = 3;
            // find the length of the extension by checking the space
            while(len){
                if(entry->ext[len - 1] == ' ')
                    len--;
                else
                    break;
            }
            if(len >= 0){
                string_concat(name.full_name, ".");
                string_concat_n(name.full_name, entry->ext, len);
            }

            if(string_icase_comp(name.full_name, component_name) == 0){
                found = 1;
                break;
            }

            memset(&name, 0, sizeof(file_name_t));
        }
        if(found)
            break;

        if(dir_end)
            break;

        // get the next cluster
        cluster_num = fat32_get_next_cluster(info->fat_lba, cluster_num);
        // check if it's the end of the cluster chain
        if(cluster_num >= 0x0FFFFFF8)
            break;
    }

    if(!found)
        return 0;

    return entry;
}

int fat32_lookup_no_cache(struct vnode *dir_node, struct vnode **target, const char *component_name){
    struct vnode *node;
    dir_entry_t *entry;
    int cluster_num;
    int type;
    char buf[BLOCK_SIZE];

    entry = fat32_lookup_dir(dir_node, component_name, buf);

    if(entry == 0){
        uart_puts("fat32_lookup_no_cache: file not found\n");
        return -1;
    }

    if(!(entry->attr & (ATTR_DIRECTORY | ATTR_ARCHIVE))){
        uart_puts("fat32_lookup_no_cache: not a directory or file\n");
        return -1;
    }

    cluster_num = (entry->starthi << 16) | entry->startlow;

    if(entry->attr & ATTR_ARCHIVE)
        type = file_t;
    else
        type = dir_t;

    node = fat32_create_vnode(dir_node, component_name, type, cluster_num, entry->size);
    *target = node;

    return 0;
}

int fat32_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name){
    if(((struct fat32_inode*)dir_node->internal)->type != dir_t){
        uart_puts("fat32_lookup: not a directory\n");
        return -1;
    }
    return fat32_lookup_no_cache(dir_node, target, component_name);
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