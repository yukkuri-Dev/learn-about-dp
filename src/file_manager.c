#include <string.h>
#include <syscalls/syscalls.h>
#include "file_manager.h"

struct file_entry file_list[MAX_FILES];
int total_files = 0;

static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

int load_files(const char *path) {
    // ローカルバッファをグローバルに移動してスタック効率を改善
    static char filename[64];
    static unsigned long type;
    static char search_path[128];
    int ret, handle;
    
    total_files = 0;
    
    // 検索パスを構築
    sprintf(search_path, "%s*", path);
    
    // すべてのファイルを読み込む
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
        return 0;
    }
    
    return -1;  // 失敗
}
