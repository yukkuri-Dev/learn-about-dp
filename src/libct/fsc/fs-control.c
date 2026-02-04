#include "libc/memmgr.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <syscalls/syscalls.h>

#define MAX_FILES 100

int ret, handle;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
                    //internal drive
    "\\\\crd0\\"   // SDカード
                    //SD card
};
unsigned long type;
char filename[64];

// ファイル情報を保存する構造体
struct file_entry {
    char name[64];
    unsigned long type;
};

// ファイルリスト返却用構造体
struct file_list_result {
    struct file_entry *entries;  // ファイルエントリの配列
    int count;                   // ファイル数
    int success;                 // 成功フラグ (0: 成功, 負数: エラー)
};

struct file_list_result get_file_list(char *search_path){
    struct file_list_result result = {0};
    struct file_entry *file_list = NULL;
    int total_files = 0;
    
    // ファイルリスト用メモリを確保
    file_list = (struct file_entry *)memmgr_alloc(sizeof(struct file_entry) * MAX_FILES);
    if (file_list == NULL) {
        result.success = -1;
        result.count = 0;
        result.entries = NULL;
        return result;
    }
    
    // ファイルリストを取得
    ret = sys_findfirst(search_path, &handle, filename, &type);
    
    if (ret == 0) {
        // 最初のファイルを保存
        strcpy(file_list[total_files].name, filename);
        file_list[total_files].type = type;
        total_files++;
        
        // 残りのファイルを順次取得して保存
        while (total_files < MAX_FILES) {
            ret = sys_findnext(handle, filename, &type);
            if (ret != 0) break;
            
            strcpy(file_list[total_files].name, filename);
            file_list[total_files].type = type;
            total_files++;
        }
        
        sys_findclose(handle);
        result.success = 0;
    } else {
        // ファイルが見つからなかった
        result.success = -2;
    }
    
    result.entries = file_list;
    result.count = total_files;
    return result;
}