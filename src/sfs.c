#include "sfs.h"

FILE *disk_fp = NULL;

void read_block(int block_num, void *buf) {
    fseek(disk_fp, block_num * BLOCK_SIZE, SEEK_SET);
    fread(buf, BLOCK_SIZE, 1, disk_fp);
}

void write_block(int block_num, const void *buf) {
    fseek(disk_fp, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(buf, BLOCK_SIZE, 1, disk_fp);
    fflush(disk_fp);
}

void read_superblock(Superblock *sb) {
    char buf[BLOCK_SIZE];
    read_block(SUPERBLOCK_BLOCK, buf);
    memcpy(sb, buf, sizeof(Superblock));
}

void write_superblock(const Superblock *sb) {
    char buf[BLOCK_SIZE] = {0};
    memcpy(buf, sb, sizeof(Superblock));
    write_block(SUPERBLOCK_BLOCK, buf);
}

void read_inode(int index, Inode *inode) {
    int offset = (INODE_START_BLOCK * BLOCK_SIZE) + (index * sizeof(Inode));
    fseek(disk_fp, offset, SEEK_SET);
    fread(inode, sizeof(Inode), 1, disk_fp);
}

void write_inode(int index, const Inode *inode) {
    int offset = (INODE_START_BLOCK * BLOCK_SIZE) + (index * sizeof(Inode));
    fseek(disk_fp, offset, SEEK_SET);
    fwrite(inode, sizeof(Inode), 1, disk_fp);
    fflush(disk_fp);
}

int alloc_inode() {
    Inode inode;
    for (int i = 0; i < MAX_FILES; i++) {
        read_inode(i, &inode);
        if (!inode.is_used) return i;
    }
    return -1;
}

void free_inode(int index) {
    Inode inode;
    memset(&inode, 0, sizeof(Inode));
    write_inode(index, &inode);

    Superblock sb;
    read_superblock(&sb);
    sb.free_inodes++;
    write_superblock(&sb);
}

void read_bitmap(Bitmap *bm) {
    char buf[BLOCK_SIZE];
    read_block(BITMAP_BLOCK, buf);
    memcpy(bm, buf, sizeof(Bitmap));
}

void write_bitmap(const Bitmap *bm) {
    char buf[BLOCK_SIZE] = {0};
    memcpy(buf, bm, sizeof(Bitmap));
    write_block(BITMAP_BLOCK, buf);
}

int alloc_block() {
    Bitmap bm;
    read_bitmap(&bm);
    for (int i = DATA_START_BLOCK; i < TOTAL_BLOCKS; i++) {
        if (bm.map[i] == 0) {
            bm.map[i] = 1;
            write_bitmap(&bm);

            Superblock sb;
            read_superblock(&sb);
            sb.free_blocks--;
            write_superblock(&sb);
            return i;
        }
    }
    return -1;
}

void free_block(int block_num) {
    Bitmap bm;
    read_bitmap(&bm);
    bm.map[block_num] = 0;
    write_bitmap(&bm);

    Superblock sb;
    read_superblock(&sb);
    sb.free_blocks++;
    write_superblock(&sb);
}

void sfs_format(const char *disk_path) {
    disk_fp = fopen(disk_path, "wb+");
    if (!disk_fp) { perror("Cannot create disk"); exit(1); }

    char zero_block[BLOCK_SIZE] = {0};
    for (int i = 0; i < TOTAL_BLOCKS; i++)
        write_block(i, zero_block);

    Superblock sb;
    sb.magic        = 0xDEADBEEF;
    sb.total_blocks = TOTAL_BLOCKS;
    sb.block_size   = BLOCK_SIZE;
    sb.total_inodes = MAX_FILES;
    sb.free_blocks  = TOTAL_BLOCKS - DATA_START_BLOCK;
    sb.free_inodes  = MAX_FILES - 1;
    write_superblock(&sb);

    Bitmap bm;
    memset(&bm, 0, sizeof(Bitmap));
    for (int i = 0; i < DATA_START_BLOCK; i++)
        bm.map[i] = 1;
    write_bitmap(&bm);

    // Root directory inode
    Inode root;
    memset(&root, 0, sizeof(Inode));
    root.is_used = 1;
    root.is_dir  = 1;
    root.parent  = -1;
    strncpy(root.name, "/", MAX_FILENAME);
    write_inode(0, &root);

    printf("Disk formatted successfully.\n");
}

int sfs_mount(const char *disk_path) {
    disk_fp = fopen(disk_path, "rb+");
    if (!disk_fp) return 0;

    Superblock sb;
    read_superblock(&sb);
    if (sb.magic != (int)0xDEADBEEF) {
        printf("Not a valid SFS disk!\n");
        fclose(disk_fp);
        return 0;
    }
    printf("Disk mounted. %d blocks free.\n", sb.free_blocks);
    return 1;
}

void sfs_unmount() {
    if (disk_fp) { fclose(disk_fp); disk_fp = NULL; }
}

int sfs_mkdir(const char *name, int parent) {
    int idx = alloc_inode();
    if (idx < 0) { printf("No free inodes!\n"); return -1; }

    Inode dir;
    memset(&dir, 0, sizeof(Inode));
    dir.is_used = 1;
    dir.is_dir  = 1;
    dir.parent  = parent;
    strncpy(dir.name, name, MAX_FILENAME);
    write_inode(idx, &dir);

    Superblock sb;
    read_superblock(&sb);
    sb.free_inodes--;
    write_superblock(&sb);

    printf("Directory '%s' created (inode %d).\n", name, idx);
    return idx;
}

int sfs_create(const char *name, int parent) {
    int idx = alloc_inode();
    if (idx < 0) { printf("No free inodes!\n"); return -1; }

    Inode file;
    memset(&file, 0, sizeof(Inode));
    file.is_used = 1;
    file.is_dir  = 0;
    file.parent  = parent;
    strncpy(file.name, name, MAX_FILENAME);
    write_inode(idx, &file);

    Superblock sb;
    read_superblock(&sb);
    sb.free_inodes--;
    write_superblock(&sb);

    printf("File '%s' created (inode %d).\n", name, idx);
    return idx;
}

int sfs_write(int inode_idx, const char *data) {
    Inode file;
    read_inode(inode_idx, &file);
    if (!file.is_used || file.is_dir) { printf("Invalid file.\n"); return -1; }

    for (int i = 0; i < file.block_count; i++)
        free_block(file.blocks[i]);
    file.block_count = 0;

    int len = strlen(data);
    int blocks_needed = (len / BLOCK_SIZE) + 1;
    if (blocks_needed > MAX_BLOCKS_PER_FILE) {
        printf("Data too large!\n"); return -1;
    }

    char block_buf[BLOCK_SIZE];
    int written = 0;
    for (int i = 0; i < blocks_needed && written < len; i++) {
        int b = alloc_block();
        if (b < 0) { printf("Disk full!\n"); return -1; }

        memset(block_buf, 0, BLOCK_SIZE);
        int chunk = (len - written) < BLOCK_SIZE ? (len - written) : BLOCK_SIZE;
        memcpy(block_buf, data + written, chunk);
        write_block(b, block_buf);

        file.blocks[file.block_count++] = b;
        written += chunk;
    }

    file.size = len;
    write_inode(inode_idx, &file);
    printf("Wrote %d bytes to '%s'.\n", len, file.name);
    return len;
}

int sfs_read(int inode_idx, char *buf, int buf_size) {
    Inode file;
    read_inode(inode_idx, &file);
    if (!file.is_used || file.is_dir) { printf("Invalid file.\n"); return -1; }

    char block_buf[BLOCK_SIZE];
    int total = 0;
    for (int i = 0; i < file.block_count && total < buf_size - 1; i++) {
        read_block(file.blocks[i], block_buf);
        int chunk = BLOCK_SIZE < (buf_size - 1 - total) ? BLOCK_SIZE : (buf_size - 1 - total);
        memcpy(buf + total, block_buf, chunk);
        total += chunk;
    }
    buf[total] = '\0';
    return total;
}

int sfs_delete(int inode_idx) {
    Inode file;
    read_inode(inode_idx, &file);
    if (!file.is_used) { printf("Nothing to delete.\n"); return -1; }

    for (int i = 0; i < file.block_count; i++)
        free_block(file.blocks[i]);

    free_inode(inode_idx);
    printf("'%s' deleted.\n", file.name);
    return 0;
}

void sfs_ls(int parent_inode) {
    Inode inode;
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        read_inode(i, &inode);
        if (inode.is_used && inode.parent == parent_inode) {
            printf("%s  %s  (%d bytes)\n",
                inode.is_dir ? "[DIR] " : "[FILE]",
                inode.name, inode.size);
            found++;
        }
    }
    if (!found) printf("(empty directory)\n");
}

int sfs_find(const char *name, int parent) {
    Inode inode;
    for (int i = 0; i < MAX_FILES; i++) {
        read_inode(i, &inode);
        if (inode.is_used && inode.parent == parent &&
            strcmp(inode.name, name) == 0)
            return i;
    }
    return -1;
}

void sfs_dump_json() {
    Superblock sb;
    read_superblock(&sb);
    Bitmap bm;
    read_bitmap(&bm);

    printf("{\n");
    printf("  \"total_blocks\": %d,\n", sb.total_blocks);
    printf("  \"block_size\": %d,\n", sb.block_size);
    printf("  \"free_blocks\": %d,\n", sb.free_blocks);
    printf("  \"free_inodes\": %d,\n", sb.free_inodes);

    printf("  \"bitmap\": [");
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        printf("%d", bm.map[i]);
        if (i < TOTAL_BLOCKS - 1) printf(",");
    }
    printf("],\n");

    printf("  \"inodes\": [\n");
    Inode inode;
    int first = 1;
    for (int i = 0; i < MAX_FILES; i++) {
        read_inode(i, &inode);
        if (!inode.is_used) continue;
        if (!first) printf(",\n");
        first = 0;
        printf("    {\"id\":%d,\"name\":\"%s\",\"is_dir\":%d,"
               "\"size\":%d,\"parent\":%d,\"block_count\":%d}",
               i, inode.name, inode.is_dir,
               inode.size, inode.parent, inode.block_count);
    }
    // ── Get inode index by path (e.g. "documents") ──────────────
int sfs_get_inode_by_name(const char *name, int parent) {
    return sfs_find(name, parent);
}
    printf("\n  ]\n}\n");
}