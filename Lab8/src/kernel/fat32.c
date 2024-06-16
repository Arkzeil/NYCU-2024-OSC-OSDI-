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
    fat32_mount_list_t *fat32_mount;
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
    fat32_mount = pool_alloc(sizeof(fat32_mount_list_t));

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

int fat32_read(struct file *file, void *buf, my_uint64_t len){

}

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

dir_entry_t *fat32_lookup_dir(struct vnode *dir_node, const char *component_name, char *buf, int *buf_lba){
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
        // called by sync, to updata buffer lba
        if(buf_lba)
            *buf_lba = lba;

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

    entry = fat32_lookup_dir(dir_node, component_name, buf, 0);

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

    *target = fat32_create_vnode(dir_node,component_name, file_t, -1, 0);

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

    *target = fat32_create_vnode(dir_node, component_name, dir_t, -1, 0);

    return 0;
}

int fat32_write_file(const void *buf, struct fat32_inode *data, unsigned int offset, my_uint64_t size){
    fat32_cache_metadata_t *metadata = 0;
    struct list_head *head = &data->file->list;
    unsigned int first_block = offset / BLOCK_SIZE;
    unsigned int current_block = 0;
    unsigned int cluster_num = data->cluster_num;
    my_uint64_t buf_offset = 0;
    int ret = 0;    // store return value
    int written = 0;
    // see if it can find the block in cache
    ret = fat32_seek_cache(data, first_block, &metadata);
    // if it can't find the block in cache, read the block from disk
    if(ret < 0)
        ret = fat32_seek_disk(data, first_block, cluster_num, &metadata);
    if(ret < 0){
        uart_puts("fat32_write_file: seek error\n");
        return 0;
    }

    while(size){
        my_uint64_t backoff = (offset + written) % BLOCK_SIZE;

        if(&metadata->list != head){
            // if the block is in cache, write the data to the cache
            ret = fat32_write_cache(data, backoff, buf, buf_offset, size, metadata);
            cluster_num = metadata->cluster_num;
            // find the next block in cache
            metadata = list_first_entry(&metadata->list, fat32_cache_metadata_t, list);
        }
        else{
            // if the block is not in cache. Read block from sdcard, create cache, then write it
            cluster_num = fat32_get_next_cluster(data->info->fat_lba, cluster_num);
            if(cluster_num >= 0x0FFFFFF8)
                break;
            ret = fat32_write_disk(data, backoff, buf, buf_offset, size, current_block, cluster_num);
        }

        if(ret < 0)
            break;
        
        buf_offset += ret;
        written += ret;
        current_block++;
        size -= ret;
    }

    return written;
}

int fat32_read_file(void *buf, struct fat32_inode *data, unsigned int offset, my_uint64_t size){
    fat32_cache_metadata_t *metadata = 0;
    struct list_head *head = &data->file->list;
    unsigned int first_block = offset / BLOCK_SIZE;
    unsigned int current_block = 0;
    unsigned int cluster_num = data->cluster_num;
    my_uint64_t buf_offset = 0;
    int ret = 0;    // store return value
    int read = 0;
    // see if it can find the block in cache
    ret = fat32_seek_cache(data, first_block, &metadata);
    // if it can't find the block in cache, read the block from disk
    if(ret < 0)
        ret = fat32_seek_disk(data, first_block, cluster_num, &metadata);
    if(ret < 0){
        uart_puts("fat32_read_file: seek error\n");
        return 0;
    }

    while(size){
        my_uint64_t backoff = (offset + read) % BLOCK_SIZE;

        if(&metadata->list != head){
            // if the block is in cache, read the data from the cache
            ret = fat32_read_cache(data, backoff, buf, buf_offset, size, metadata);
            cluster_num = metadata->cluster_num;
            // find the next block in cache
            metadata = list_first_entry(&metadata->list, fat32_cache_metadata_t, list);
        }
        else{
            // if the block is not in cache. Read block from sdcard, create cache, then read it
            cluster_num = fat32_get_next_cluster(data->info->fat_lba, cluster_num);
            if(cluster_num >= 0x0FFFFFF8)
                break;

            ret = fat32_read_disk(data, backoff, buf, buf_offset, size, current_block, cluster_num);
        }

        if(ret < 0)
            break;
        
        buf_offset += ret;
        read += ret;
        current_block++;
        size -= ret;
    }

    return read;
}

// write data to the cache
int fat32_write_cache(struct fat32_inode *data, my_uint64_t backoff, const unsigned char *buf, my_uint64_t buf_offset, unsigned int size, fat32_cache_metadata_t *metadata){
    int write_size;
    // if the cache is not updated, read the block from the disk
    if(!metadata->updated){
        fat32_info_t *info = data->info;
        int lba = info->cluster_lba + (metadata->cluster_num - 2) * info->bs.sectors_per_cluster;
        
        readblock(lba, metadata->buf);
        metadata->updated = 1;
    }
    // if the size is larger than the remaining block size, just write the remaining block to cache
    if(size > BLOCK_SIZE - backoff)
        write_size = BLOCK_SIZE - backoff;
    else
        write_size = size;

    memcpy(&metadata->buf[backoff], &buf[buf_offset], write_size);

    return write_size;
}
// write to cache and make the cache dirty for future sync
int fat32_write_disk(struct fat32_inode *data, my_uint64_t backoff, const unsigned char *buf, my_uint64_t buf_offset, unsigned int size, unsigned int offset, unsigned int cluster_num){
    struct list_head *pos = &data->file->list;
    fat32_info_t *info = data->info;
    fat32_cache_metadata_t *metadata = pool_alloc(sizeof(fat32_cache_metadata_t));
    unsigned int lba, write_size;

    if(size > BLOCK_SIZE - backoff)
        write_size = BLOCK_SIZE - backoff;
    else
        write_size = size;

    if(cluster_num >= 0x0FFFFFF8){
        // should return error?
        memset(metadata->buf, 0, BLOCK_SIZE);
    }
    else{
        lba = info->cluster_lba + (cluster_num - 2) * info->bs.sectors_per_cluster;
        readblock(lba, metadata->buf);
    }

    memcpy(&metadata->buf[backoff], &buf[buf_offset], write_size);

    metadata->updated = 1;
    metadata->dirty = 1;
    metadata->offset = offset;
    metadata->cluster_num = cluster_num;

    list_add_tail(&metadata->list, pos);    // add the new block to the end of the cache list

    return write_size;
}

// read data from the cache
int fat32_read_cache(struct fat32_inode *data, my_uint64_t backoff, unsigned char *buf, my_uint64_t buf_offset, unsigned int size, fat32_cache_metadata_t *metadata){
    int read_size;
    // if the cache is not updated, read the block from the disk
    if(!metadata->updated){
        fat32_info_t *info = data->info;
        int lba = info->cluster_lba + (metadata->cluster_num - 2) * info->bs.sectors_per_cluster;
        
        readblock(lba, metadata->buf);
        metadata->updated = 1;
    }
    // if the size is larger than the remaining block size, just read the remaining block from cache
    if(size > BLOCK_SIZE - backoff)
        read_size = BLOCK_SIZE - backoff;
    else
        read_size = size;

    memcpy(&buf[buf_offset], &metadata->buf[backoff], read_size);

    return read_size;
}

int fat32_read_disk(struct fat32_inode *data, my_uint64_t backoff, unsigned char *buf, my_uint64_t buf_offset, unsigned int size, unsigned int offset, unsigned int cluster_num){
    struct list_head *pos = &data->file->list;
    fat32_info_t *info = data->info;
    fat32_cache_metadata_t *metadata = pool_alloc(sizeof(fat32_cache_metadata_t));
    unsigned int lba, read_size;

    if(size > BLOCK_SIZE - backoff)
        read_size = BLOCK_SIZE - backoff;
    else
        read_size = size;

    lba = info->cluster_lba + (cluster_num - 2) * info->bs.sectors_per_cluster;
    readblock(lba, metadata->buf);

    memcpy(&buf[buf_offset], &metadata->buf[backoff], read_size);

    metadata->updated = 1;
    metadata->offset = offset;
    metadata->cluster_num = cluster_num;

    list_add_tail(&metadata->list, pos);    // add the new block to the end of the cache list

    return read_size;
}
// using the inode and the offset to find the cache metadata
// offset is the desired block number
int fat32_seek_cache(struct fat32_inode *data, unsigned int offset, fat32_cache_metadata_t **metadata){
    fat32_cache_metadata_t *temp;
    // get the list of the 
    struct list_head *pos = &data->file->list;

    if(list_empty(pos)){
        uart_puts("fat32_seek_cache: cache list is empty\n");
        return -1;
    }

    list_for_each_entry(temp, pos, list){
        metadata = temp;
        if(temp->offset == offset){ // if the offset is the same, return the metadata
            *metadata = temp;
            return 0;
        }
    }

    return -1;
}
// using the inode and the offset to find the data on disk,and create a new corresponding cache
int fat32_seek_disk(struct fat32_inode *data, unsigned int offset, unsigned int cluster_num, fat32_cache_metadata_t **metadata){
    fat32_info_t *info = data->info;
    unsigned int current_offset, current_cluster_num;
    // if the metadata is not empty(exist in cache), get the offset and cluster number
    if(*metadata){
        current_offset = (*metadata)->offset;
        current_cluster_num = (*metadata)->cluster_num;

        if(current_offset == offset)
            return 0;
        
        current_offset++;
        current_cluster_num = fat32_get_next_cluster(info->fat_lba, current_cluster_num);

        if(current_cluster_num >= 0x0FFFFFF8)
            return -1;
    }
    else{
        current_offset = 0;
        current_cluster_num = cluster_num;
    }
    // create a new block in cache
    while(1){
        fat32_cache_metadata_t *temp = pool_alloc(sizeof(fat32_cache_metadata_t));

        temp->offset = current_offset;
        temp->cluster_num = current_cluster_num;
        temp->updated = 0;
        temp->dirty = 1;
        // put the new block to the end of the list of cache
        list_add_tail(&temp->list, &data->file->list);

        *metadata = temp;

        if(current_offset == offset)
            return 0;

        current_offset++;
        current_cluster_num = fat32_get_next_cluster(info->fat_lba, current_cluster_num);

        if(current_cluster_num >= 0x0FFFFFF8)
            return -1;
    }
}

void fat32_sync_dir(struct vnode *dir_node){
    struct fat32_inode *data = (struct fat32_inode*)dir_node->internal; 
    struct fat32_inode *entry;
    struct list_head *head = &data->dir->list;
    dir_entry_t *dir;
    dir_long_entry_t *long_dir;
    unsigned int cluster_num = data->cluster_num;
    int lba;
    int index = 0;
    int long_file_index = 1;
    unsigned char buf[BLOCK_SIZE];

    if(cluster_num >= 0x0FFFFFF8){
        uart_puts("fat32_sync_dir: end of cluster chain\n");
        return;
    }

    lba = data->info->cluster_lba + (cluster_num - 2) * data->info->bs.sectors_per_cluster;
    readblock(lba, buf);

    list_for_each_entry(entry, head, list){
        dir_entry_t *ori_dir;
        const char *name = entry->name;
        const char *ext;
        int i;
        int LFN, name_len, ext_pos, buf_lba;
        unsigned char temp_buf[BLOCK_SIZE];
        
        ori_dir = fat32_lookup_dir(dir_node, name, temp_buf, &buf_lba);
        // if the entry exist(old ffile), update its size
        if(ori_dir){
            if(entry->type == file_t){
                ori_dir->size = entry->file->size;
                writeblock(buf_lba, temp_buf);
            }
            continue;
        }
        // if the entry doesn't exist(new file), create a new entry
        ext = 0;
        ext_pos = -1;

        do {
            name_len = string_len(name);
            if (name_len >= 13) {
                LFN = 1;
                break;
            }
            for (i = 0; i < name_len; i++) {
                // get idx of extension name
                if (name[name_len - 1 - i] == '.') {
                    break;
                }
            }
            if (i < name_len) {
                ext = &name[name_len - i];
                ext_pos = name_len - 1 - i;
            }
            if (i >= 4) {
                LFN = 1;
                break;
            }
            if (name_len - 1 - i > 8) {
                // SFN: 8.3
                LFN = 1;
                break;
            }

            LFN = 0;
        }while(0);

        // Seek idx to the end of dir
        while(1){
            dir = (dir_entry_t*)(&buf[sizeof(dir_entry_t) * index]);
            // if the entry is empty, break
            if (dir->name[0] == 0){
                break;
            }

            index++;

            if (index >= 16) { // if idx is over Directory Entry, create a new one
                unsigned int new_cluster_num;

                writeblock(lba, buf);

                new_cluster_num = get_next_cluster(data->info->fat_lba, cluster_num);
                if (new_cluster_num >= 0x0FFFFFF8)
                    new_cluster_num = alloc_cluster(data->info, cluster_num);
                
                cluster_num = new_cluster_num;

                lba = data->info->cluster_lba + (cluster_num - 2) * data->info->bs.sectors_per_cluster;

                readblock(lba, buf);
                index = 0;
            }
        }

        // Write LFN
        if(LFN) {
            int ord;
            int first;

            // the number of LFN entries required
            ord = ((name_len - 1) / 13) + 1;
            first = 0x40; // LAST_LONG_ENTRY flag in LFN LDIR_Ord

            for (; ord > 0; --ord) {
                int end;
                long_dir = (struct long_dir_t *)(&buf[sizeof(dir_long_entry_t) * index]);

                long_dir->order = first | ord;
                long_dir->attr = ATTR_LONG_NAME;
                long_dir->type = 0;
                // TODO: Calculate checksum, SFN + LFN
                long_dir->checksum = 0;
                long_dir->start_cluster = 0;

                first = 0;
                end = 0;
                // 1~10
                for (i = 0; i < 10; i += 2) {
                    if (end) { // padding 0xff if filename end
                        long_dir->name1[i] = 0xff;
                        long_dir->name1[i + 1] = 0xff;
                    } else {
                        long_dir->name1[i] = name[(ord - 1) * 13 + i / 2];
                        long_dir->name1[i + 1] = 0;
                        if (long_dir->name1[i] == 0) {
                            end = 1;
                        }
                    }
                }
                // 11~22
                for (i = 0; i < 12; i += 2) {
                    if (end) {
                        long_dir->name2[i] = 0xff;
                        long_dir->name2[i + 1] = 0xff;
                    } else {
                        long_dir->name2[i] = name[(ord - 1) * 13 + 5 + i / 2];
                        long_dir->name2[i + 1] = 0;
                        if (long_dir->name2[i] == 0) {
                            end = 1;
                        }
                    }
                }
                // 23~26
                for (i = 0; i < 4; i += 2) {
                    if (end) {
                        long_dir->name3[i] = 0xff;
                        long_dir->name3[i + 1] = 0xff;
                    } else {
                        long_dir->name3[i] = name[(ord - 1) * 13 + 11 + i / 2];
                        long_dir->name3[i + 1] = 0;
                        if (long_dir->name3[i] == 0) {
                            end = 1;
                        }
                    }
                }

                index++;

                if (index >= 16) {
                    unsigned int newcid;

                    writeblock(lba, buf);

                    newcid = get_next_cluster(data->info->fat_lba, cluster_num);
                    if (newcid >= 0x0FFFFFF8) {
                        newcid = alloc_cluster(data->info, cluster_num);
                    }

                    cluster_num = newcid;

                    lba = data->info->cluster_lba + (cluster_num - 2) * data->info->bs.sectors_per_cluster;
                    readblock(lba, buf);

                    index = 0;
                }
            }
        }

        // Write SFN
        dir = (struct dir_t *)(&buf[sizeof(dir_entry_t) * index]);

        // TODO: Set these properties properly
        dir->lcase = 0;
        dir->ctime_cs = 0;
        dir->ctime = 0;
        dir->cdate = 0;
        dir->adate = 0;
        dir->time = 0;
        dir->date = 0;

        if (entry->type == dir_t) {
            dir->attr = ATTR_DIRECTORY;
            dir->size = 0;
        } else {
            dir->attr = ATTR_ARCHIVE;
            dir->size = entry->file->size;
        }

        if (entry->cluster_num >= 0x0FFFFFF8) {
            entry->cluster_num = alloc_cluster(data->info, 0);
        }

        dir->starthi = (entry->cluster_num >> 16) & 0xffff;
        dir->startlow = entry->cluster_num & 0xffff;

        if (LFN) {
            int lfni;

            // Creating SFN body
            // TODO: handle lfnidx
            for (i = 7, lfni = long_file_index; i >= 0 && lfni;) {
                dir->name[i--] = '0' + lfni % 10;
                lfni /= 10;
            }

            long_file_index++;
            // numeric-tail: ~n (1 <= n <= 6)
            dir->name[i--] = '~';

            // TODO: handle letter case
            memcpy((void *)dir->name, name, i + 1);
        } else {
            // TODO: handle letter case
            for (i = 0; i != ext_pos && name[i]; ++i) {
                dir->name[i] = name[i];
            }

            for (; i < 8; ++i) {
                dir->name[i] = ' '; // in SFN, each part is padded with space
            }
        }

        // SFN format: 8.3
        // TODO: handle letter case
        for (i = 0; i < 3 && ext[i]; ++i) {
            dir->name[8 + i] = ext[i];
        }

        for (; i < 3; ++i) {
            dir->name[8 + i] = ' ';
        }

        index += 1;

        if (index >= 16) {
            int newcid;

            writeblock(lba, buf);

            newcid = get_next_cluster(data->info->fat_lba, cluster_num);
            if (newcid >= 0x0FFFFFF8) {
                newcid = alloc_cluster(data->info, cluster_num);
            }

            cluster_num = newcid;

            lba = data->info->cluster_lba + (cluster_num - 2) * data->info->bs.sectors_per_cluster;

            // TODO: Cache data block of directory
            readblock(lba, buf);

            index = 0;
        }
    }
}

void fat32_sync_file(struct vnode *file_node){

}

void fat32_sync_all(struct vnode *dir_node){

}