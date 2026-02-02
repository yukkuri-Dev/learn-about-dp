#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syscalls/syscalls.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320
#define MAX_DISPLAY 15  // 画面に表示する最大ファイル数
#define MAX_FILES 100   // 読み込む最大ファイル数

// UI Layout Constants
#define HEADER_Y 10
#define FILE_LIST_START_Y 30
#define LINE_SPACING 2
#define FOOTER_OFFSET 10
#define CURSOR_X 0
#define TEXT_X 10
#define DEBUG_INFO_X 400
#define DEBUG_INFO_Y 10
#define DEBUG_INFO_WIDTH 120
#define DEBUG_INFO_HEIGHT 20
#define MAX_PATH_LENGTH 256

// ファイル情報を保存する構造体
struct file_entry {
    char name[64];
    unsigned long type;
};

// Application state structure
struct app_state {
    struct file_entry file_list[MAX_FILES];
    int total_files;
    int scroll_offset;
    int selected_index;
    int prev_selected_index;
    int refresh_needed;
};

// Global application state
struct app_state app_state = {0};

// Temporary variables for file operations
char filename[64];
unsigned long type;
char search_path[128];
int ret, handle;
static char *drive[2] = {
    "\\\\drv0\\",  // 内蔵ドライブ
    "\\\\crd0\\"   // SDカード
};

// 選択されたファイル名を取得
char* get_selected_filename(void) {
    if (app_state.selected_index >= 0 && app_state.selected_index < app_state.total_files) {
        return app_state.file_list[app_state.selected_index].name;
    }
    return NULL;
}

// 選択されたファイルタイプを取得
unsigned long get_selected_filetype(void) {
    if (app_state.selected_index >= 0 && app_state.selected_index < app_state.total_files) {
        return app_state.file_list[app_state.selected_index].type;
    }
    return 0;
}

// Safe string copy with null termination
void safe_string_copy(char *dest, const char *src, size_t dest_size) {
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

// Clear screen
void clear_screen(void) {
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// Draw single file entry
void draw_file_entry(int y_position, const struct file_entry *entry) {
    char display_name[80];
    
    set_pen(create_rgb16(255, 255, 255));
    if (entry->type == 5) {
        sprintf(display_name, "[DIR]  %s", entry->name);
    } else if (entry->type == 1) {
        sprintf(display_name, "[FILE] %s", entry->name);
    } else {
        sprintf(display_name, "[?]    %s", entry->name);
    }
    render_text(TEXT_X, y_position, display_name);
}

// Draw file list
void draw_file_list(int start_index, int count) {
    struct font *fnt = get_font();
    int line_height = fnt->height + LINE_SPACING;
    
    for (int i = 0; i < count && (start_index + i) < app_state.total_files; i++) {
        int y_pos = FILE_LIST_START_Y + i * line_height;
        draw_file_entry(y_pos, &app_state.file_list[start_index + i]);
    }
}

// Draw header with current path
void draw_header(const char *path) {
    char display_name[80];
    set_pen(create_rgb16(255, 255, 0));  // Yellow
    sprintf(display_name, "=== File List: %s ===", path);
    render_text(TEXT_X, HEADER_Y, display_name);
}

// Draw file count
void draw_file_count(int count) {
    struct font *fnt = get_font();
    int line_height = fnt->height + LINE_SPACING;
    char display_name[80];
    
    set_pen(create_rgb16(0, 255, 255));  // Cyan
    sprintf(display_name, "Total: %d files", count);
    render_text(TEXT_X, FILE_LIST_START_Y + MAX_DISPLAY * line_height + FOOTER_OFFSET, display_name);
}

// Draw footer with instructions
void draw_footer(void) {
    struct font *fnt = get_font();
    char *exit_msg = "Press POWER to exit. Press BACK to root.";
    
    set_pen(create_rgb16(0, 255, 0));  // Green
    render_text(
        (SCREEN_WIDTH - strlen(exit_msg) * fnt->width) / 2,
        SCREEN_HEIGHT - fnt->height - FOOTER_OFFSET,
        exit_msg
    );
}

// Draw cursor at specified screen position
void draw_cursor(int screen_position) {
    struct font *fnt = get_font();
    int line_height = fnt->height + LINE_SPACING;
    
    set_pen(create_rgb16(0, 0, 255));  // Blue
    render_text(CURSOR_X, FILE_LIST_START_Y + screen_position * line_height, ">");
}

// Clear cursor at specified screen position
void clear_cursor(int screen_position) {
    struct font *fnt = get_font();
    int line_height = fnt->height + LINE_SPACING;
    
    set_pen(create_rgb16(0, 0, 0));  // Black
    render_text(CURSOR_X, FILE_LIST_START_Y + screen_position * line_height, ">");
}

// Calculate screen position from index
int calculate_screen_position(int index) {
    return index - app_state.scroll_offset;
}

// Draw debug info
void draw_debug_info(void) {
    char display_name[80];
    struct font *fnt = get_font();
    
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(DEBUG_INFO_X, DEBUG_INFO_Y, DEBUG_INFO_WIDTH, DEBUG_INFO_HEIGHT);
    
    set_pen(create_rgb16(255, 255, 255));
    sprintf(display_name, "Sel: %d/%d", app_state.selected_index + 1, app_state.total_files);
    render_text(DEBUG_INFO_X, DEBUG_INFO_Y, display_name);
}

// Load directory contents
int load_directory(const char *path) {
    app_state.total_files = 0;
    
    ret = sys_findfirst(path, &handle, filename, &type);
    if (ret != 0) {
        return -1;
    }
    
    // Store first file
    safe_string_copy(app_state.file_list[app_state.total_files].name, filename, 64);
    app_state.file_list[app_state.total_files].type = type;
    app_state.total_files++;
    
    // Load remaining files
    while (app_state.total_files < MAX_FILES) {
        ret = sys_findnext(handle, filename, &type);
        if (ret != 0) break;
        
        safe_string_copy(app_state.file_list[app_state.total_files].name, filename, 64);
        app_state.file_list[app_state.total_files].type = type;
        app_state.total_files++;
    }
    
    sys_findclose(handle);
    return 0;
}

// Clear file list area
void clear_file_list_area(void) {
    struct font *fnt = get_font();
    int line_height = fnt->height + LINE_SPACING;
    
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, FILE_LIST_START_Y, SCREEN_WIDTH, MAX_DISPLAY * line_height);
}

// Redraw entire screen
void redraw_screen(const char *path) {
    clear_screen();
    draw_header(path);
    
    if (app_state.total_files > 0) {
        draw_file_list(app_state.scroll_offset, MAX_DISPLAY);
        draw_file_count(app_state.total_files);
    } else {
        set_pen(create_rgb16(255, 0, 0));  // Red
        render_text(TEXT_X, FILE_LIST_START_Y, "No files found in this directory");
    }
    
    draw_footer();
    lcdc_copy_vram();
}

// Update cursor display
void update_cursor_display(void) {
    int prev_screen_pos = calculate_screen_position(app_state.prev_selected_index);
    int screen_pos = calculate_screen_position(app_state.selected_index);
    
    // Clear previous cursor
    if (prev_screen_pos >= 0 && prev_screen_pos < MAX_DISPLAY) {
        clear_cursor(prev_screen_pos);
    }
    
    // Draw new cursor
    if (screen_pos >= 0 && screen_pos < MAX_DISPLAY) {
        draw_cursor(screen_pos);
    }
    
    draw_debug_info();
    lcdc_copy_vram();
    app_state.refresh_needed = 0;
}

// Navigate to directory
void navigate_to_directory(const char *selected_file, char *current_path) {
    char *new_path = memmgr_alloc(MAX_PATH_LENGTH);
    char *search_str = memmgr_alloc(MAX_PATH_LENGTH);
    
    if (new_path == NULL || search_str == NULL) {
        if (new_path) memmgr_free(new_path);
        if (search_str) memmgr_free(search_str);
        return;
    }
    
    // Get directory path from current path (remove "*")
    strcpy(new_path, current_path);
    char *star_pos = strchr(new_path, '*');
    if (star_pos != NULL) {
        *star_pos = '\0';
    }
    
    // Add selected directory
    strcat(new_path, selected_file);
    strcat(new_path, "\\");
    
    // Create search path with "*"
    sprintf(search_str, "%s*", new_path);
    
    // Reset navigation state
    app_state.selected_index = 0;
    app_state.prev_selected_index = 0;
    app_state.scroll_offset = 0;
    
    // Load new directory
    if (load_directory(search_str) == 0) {
        strcpy(current_path, search_str);
        redraw_screen(current_path);
        app_state.refresh_needed = 1;
    }
    
    memmgr_free(new_path);
    memmgr_free(search_str);
}

// Move selection up
void move_selection_up(void) {
    if (app_state.selected_index <= 0) {
        return;
    }
    
    app_state.prev_selected_index = app_state.selected_index;
    app_state.selected_index--;
    
    // Check if scrolling is needed
    if (app_state.selected_index < app_state.scroll_offset) {
        app_state.scroll_offset = app_state.selected_index;
        clear_file_list_area();
        draw_file_list(app_state.scroll_offset, MAX_DISPLAY);
    }
    
    app_state.refresh_needed = 1;
}

// Move selection down
void move_selection_down(void) {
    if (app_state.selected_index >= app_state.total_files - 1) {
        return;
    }
    
    app_state.prev_selected_index = app_state.selected_index;
    app_state.selected_index++;
    
    // Check if scrolling is needed
    if (app_state.selected_index >= app_state.scroll_offset + MAX_DISPLAY) {
        app_state.scroll_offset = app_state.selected_index - MAX_DISPLAY + 1;
        clear_file_list_area();
        draw_file_list(app_state.scroll_offset, MAX_DISPLAY);
    }
    
    app_state.refresh_needed = 1;
}

// Handle ENTER key press
void handle_enter_key(char *current_path) {
    char *selected_file = get_selected_filename();
    unsigned long selected_type = get_selected_filetype();
    
    if (selected_file != NULL && selected_type == 5) {
        navigate_to_directory(selected_file, current_path);
    }
    
    // Wait for key release
    while (get_key_state(KEY_ENTER)) {
        keypad_read();
    }
}

// Handle UP key press
void handle_up_key(void) {
    move_selection_up();
    
    // Wait for key release
    while (get_key_state(KEY_UP)) {
        keypad_read();
    }
}

// Handle DOWN key press
void handle_down_key(void) {
    move_selection_down();
    
    // Wait for key release
    while (get_key_state(KEY_DOWN)) {
        keypad_read();
    }
}

// Main input handling loop
void handle_input(char *current_path) {
    while (1) {
        keypad_read();
        
        if (get_key_state(KEY_POWER)) {
            return;
        }
        
        if (get_key_state(KEY_BACK)) {
            return;
        }
        
        if (get_key_state(KEY_ENTER)) {
            handle_enter_key(current_path);
        }
        
        if (get_key_state(KEY_UP)) {
            handle_up_key();
        }
        
        if (get_key_state(KEY_DOWN)) {
            handle_down_key();
        }
        
        if (app_state.refresh_needed) {
            update_cursor_display();
        }
    }
}

// Initialize application
int initialize_app(void) {
    memmgr_init();
    
    // Initialize state
    app_state.total_files = 0;
    app_state.scroll_offset = 0;
    app_state.selected_index = 0;
    app_state.prev_selected_index = 0;
    app_state.refresh_needed = 1;
    
    return 0;
}

// Draw initial screen
void draw_initial_screen(const char *path) {
    clear_screen();
    draw_header(path);
    
    if (app_state.total_files > 0) {
        draw_file_list(0, MAX_DISPLAY);
        draw_file_count(app_state.total_files);
        
        // Draw initial cursor
        draw_cursor(0);
    } else {
        set_pen(create_rgb16(255, 0, 0));  // Red
        render_text(TEXT_X, FILE_LIST_START_Y, "No files found!");
    }
    
    draw_footer();
    lcdc_copy_vram();
}

int main(void) {
    // Initialize application
    if (initialize_app() != 0) {
        return -1;
    }
    
    // Allocate memory for path
    size_t path_len = strlen(drive[0]) + strlen("*") + 1;
    char *path = memmgr_alloc(path_len);
    if (path == NULL) {
        set_pen(create_rgb16(255, 0, 0));  // Red
        render_text(TEXT_X, HEADER_Y, "Memory allocation failed!");
        lcdc_copy_vram();
        while (1) {
            keypad_read();
            if (get_key_state(KEY_POWER)) {
                return -2;
            }
        }
    }
    
    // Create initial path
    sprintf(path, "%s%s", drive[0], "*");
    
    // Load initial directory
    if (load_directory(path) == 0) {
        draw_initial_screen(path);
    } else {
        clear_screen();
        draw_header(path);
        set_pen(create_rgb16(255, 0, 0));  // Red
        render_text(TEXT_X, FILE_LIST_START_Y, "No files found!");
        draw_footer();
        lcdc_copy_vram();
    }
    
    // Main input loop
    handle_input(path);
    
    // Cleanup
    memmgr_free(path);
    return 0;
}
