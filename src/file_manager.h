#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#define MAX_FILES 100

// ファイル情報を保存する構造体
struct file_entry {
    char name[64];
    unsigned long type;
};

extern struct file_entry file_list[MAX_FILES];
extern int total_files;

// ファイルを読み込む関数
int load_files(const char *path);

#endif
