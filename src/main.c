#include "sfs.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: sfs <command> [args]\n");
        printf("Commands:\n");
        printf("  format                    - create a new disk\n");
        printf("  ls [parent_id]            - list directory\n");
        printf("  mkdir <name> [parent_id]  - create a directory\n");
        printf("  create <name> [parent_id] - create a file\n");
        printf("  write <name> <text> [parent_id] - write to file\n");
        printf("  read <name> [parent_id]   - read a file\n");
        printf("  delete <name> [parent_id] - delete file or dir\n");
        printf("  find <name> [parent_id]   - find inode index\n");
        printf("  json                      - dump state as JSON\n");
        return 1;
    }

    const char *disk = "disk.img";

    if (strcmp(argv[1], "format") == 0) {
        sfs_format(disk);
        return 0;
    }

    if (!sfs_mount(disk)) {
        printf("No disk found. Run 'format' first.\n");
        return 1;
    }

    if (strcmp(argv[1], "ls") == 0) {
        int parent = argc >= 3 ? atoi(argv[2]) : 0;
        sfs_ls(parent);

    } else if (strcmp(argv[1], "mkdir") == 0 && argc >= 3) {
        int parent = argc >= 4 ? atoi(argv[3]) : 0;
        sfs_mkdir(argv[2], parent);

    } else if (strcmp(argv[1], "create") == 0 && argc >= 3) {
        int parent = argc >= 4 ? atoi(argv[3]) : 0;
        sfs_create(argv[2], parent);

    } else if (strcmp(argv[1], "write") == 0 && argc >= 4) {
        int parent = argc >= 5 ? atoi(argv[4]) : 0;
        int idx = sfs_find(argv[2], parent);
        if (idx < 0) printf("File not found.\n");
        else sfs_write(idx, argv[3]);

    } else if (strcmp(argv[1], "read") == 0 && argc >= 3) {
        int parent = argc >= 4 ? atoi(argv[3]) : 0;
        int idx = sfs_find(argv[2], parent);
        if (idx < 0) { printf("File not found.\n"); }
        else {
            char buf[4096];
            sfs_read(idx, buf, sizeof(buf));
            printf("%s\n", buf);
        }

    } else if (strcmp(argv[1], "delete") == 0 && argc >= 3) {
        int parent = argc >= 4 ? atoi(argv[3]) : 0;
        int idx = sfs_find(argv[2], parent);
        if (idx < 0) printf("Not found.\n");
        else sfs_delete(idx);

    } else if (strcmp(argv[1], "find") == 0 && argc >= 3) {
        int parent = argc >= 4 ? atoi(argv[3]) : 0;
        int idx = sfs_find(argv[2], parent);
        printf("%d\n", idx);

    } else if (strcmp(argv[1], "json") == 0) {
        sfs_dump_json();

    } else {
        printf("Unknown command.\n");
    }

    sfs_unmount();
    return 0;
}