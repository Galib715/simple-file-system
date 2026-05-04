#ifndef SFS_H
#define SFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ── Disk constants ──────────────────────────────────────────
#define BLOCK_SIZE          512
#define TOTAL_BLOCKS        128
#define MAX_FILES           32
#define MAX_FILENAME        20
#define MAX_BLOCKS_PER_FILE 4

// Block layout:
// Block 0       → Superblock
// Blocks 1–4    → Inode table
// Block 5       → Free bitmap
// Blocks 6–127  → Data blocks

#define SUPERBLOCK_BLOCK  0
#define INODE_START_BLOCK 1
#define INODE_BLOCKS      4
#define BITMAP_BLOCK      5
#define DATA_START_BLOCK  6

// ── Superblock (fits in 512 bytes easily) ───────────────────
typedef struct {
    int magic;
    int total_blocks;
    int block_size;
    int total_inodes;
    int free_blocks;
    int free_inodes;
} Superblock;

// ── Inode ───────────────────────────────────────────────────
// Size = 20 + 4 + 4*4 + 4 + 4 + 4 = 52 bytes
// 512 / 52 = 9 inodes per block → safe!
typedef struct {
    char name[MAX_FILENAME]; // 20 bytes
    int  is_used;            // 4 bytes
    int  blocks[MAX_BLOCKS_PER_FILE]; // 16 bytes
    int  is_dir;             // 4 bytes
    int  size;               // 4 bytes
    int  parent;             // 4 bytes
    int  block_count;        // 4 bytes
} Inode;                     // total: 56 bytes

// ── Bitmap ──────────────────────────────────────────────────
typedef struct {
    char map[TOTAL_BLOCKS];
} Bitmap;

// ── Function declarations ────────────────────────────────────
void sfs_format(const char *disk_path);
int  sfs_mount(const char *disk_path);
void sfs_unmount();

void read_block(int block_num, void *buf);
void write_block(int block_num, const void *buf);

void read_superblock(Superblock *sb);
void write_superblock(const Superblock *sb);

void read_inode(int index, Inode *inode);
void write_inode(int index, const Inode *inode);
int  alloc_inode();
void free_inode(int index);

void read_bitmap(Bitmap *bm);
void write_bitmap(const Bitmap *bm);
int  alloc_block();
void free_block(int block_num);

int  sfs_mkdir(const char *name, int parent);
int  sfs_create(const char *name, int parent);
int  sfs_write(int inode_idx, const char *data);
int  sfs_read(int inode_idx, char *buf, int buf_size);
int  sfs_delete(int inode_idx);
void sfs_ls(int parent_inode);
int  sfs_find(const char *name, int parent);
void sfs_dump_json();

extern FILE *disk_fp;

#endif