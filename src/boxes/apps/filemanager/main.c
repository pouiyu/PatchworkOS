#include <patchwork/patchwork.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/defs.h>

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480
#define ITEM_HEIGHT   25

typedef struct file_entry {
    char name[256];
    char path[512];
    int is_dir;
} file_entry_t;

typedef struct file_manager_data {
    file_entry_t entries[256];
    int entry_count;
    int selected_index;
    char current_path[512];
    font_t* font;
    int scroll_offset;
    display_t* display;
    window_t* window;
} file_manager_data_t;

static void load_directory(file_manager_data_t* data, const char* path)
{
    strncpy(data->current_path, path, sizeof(data->current_path) - 1);
    data->current_path[sizeof(data->current_path) - 1] = '\0';
    
    fd_t dir_fd = open(path);
    if (dir_fd == ERR) {
        printf("File Manager: Failed to open directory: %s\n", path);
        return;
    }
    
    dirent_t* dir_entries = NULL;
    uint64_t count = 0;
    if (readdir(dir_fd, &dir_entries, &count) == ERR || dir_entries == NULL) {
        printf("File Manager: Failed to read directory\n");
        close(dir_fd);
        return;
    }
    
    data->entry_count = 0;
    memset(data->entries, 0, sizeof(data->entries));
    
    for (uint64_t i = 0; i < count && data->entry_count < 256; i++) {
        const char* name = strrchr(dir_entries[i].path, '/');
        if (name == NULL) 
            name = dir_entries[i].path;
        else 
            name++;
        
        if (strlen(name) == 0) continue;
        if (strcmp(name, ".") == 0) continue;
        if (strcmp(name, "..") == 0) continue;
        
        strncpy(data->entries[data->entry_count].name, name, sizeof(data->entries[data->entry_count].name) - 1);
        snprintf(data->entries[data->entry_count].path, 
                 sizeof(data->entries[data->entry_count].path), 
                 "%s/%s", path, name);
        data->entries[data->entry_count].is_dir = (dir_entries[i].type == 2);
        data->entry_count++;
    }
    
    free(dir_entries);
    close(dir_fd);
    
    data->selected_index = 0;
    data->scroll_offset = 0;
    
    printf("File Manager: Loaded %d entries from %s\n", data->entry_count, path);
}

static uint64_t procedure(window_t* win, element_t* elem, const event_t* event)
{
    switch (event->type) {
        case EVENT_LIB_INIT: {
            // 分配并初始化数据
            file_manager_data_t* data = malloc(sizeof(file_manager_data_t));
            if (data == NULL) {
                printf("File Manager: Failed to allocate data\n");
                return ERR;
            }
            memset(data, 0, sizeof(file_manager_data_t));
            
            // 初始化显示和字体
            data->display = window_get_display(win);
            data->window = win;
            
            // 加载字体
            data->font = font_new(data->display, "default", "regular", 14);
            if (data->font == NULL) {
                printf("File Manager: Default font not found, trying alternative...\n");
                data->font = font_new(data->display, "firacode", "retina", 14);
            }
            if (data->font == NULL) {
                printf("File Manager: Warning - No font loaded\n");
            }
            
            // 绑定数据到元素
            element_set_private(elem, data);
            
            // 加载根目录
            load_directory(data, "/");
            printf("File Manager: Initialized successfully\n");
            break;
        }
        
        case EVENT_LIB_DEINIT: {
            file_manager_data_t* data = element_get_private(elem);
            if (data != NULL) {
                if (data->font != NULL) {
                    font_free(data->font);
                }
                free(data);
            }
            break;
        }
        
        case EVENT_LIB_REDRAW: {
            file_manager_data_t* data = element_get_private(elem);
            if (data == NULL) break;
            
            drawable_t draw;
            element_draw_begin(elem, &draw);
            
            // 获取内容区域
            rect_t win_rect = element_get_content_rect(elem);
            
            // 绘制主背景
            draw_rect(&draw, &win_rect, 0xFFEEEEEE);
            
            // 绘制路径栏
            rect_t path_rect = {10, 10, WINDOW_WIDTH - 20, 30};
            draw_rect(&draw, &path_rect, 0xFFDDDDDD);
            
            if (data->font != NULL) {
                point_t path_pos = {15, 22};
                draw_string(&draw, data->font, &path_pos, 0xFF000000, 
                           data->current_path, strlen(data->current_path));
            }
            
            // 绘制文件列表
            rect_t list_rect = {10, 50, WINDOW_WIDTH - 20, WINDOW_HEIGHT - 90};
            draw_rect(&draw, &list_rect, 0xFFFFFFFF);
            
            int visible_items = (list_rect.bottom - list_rect.top) / ITEM_HEIGHT;
            
            for (int i = data->scroll_offset; 
                 i < data->entry_count && i < data->scroll_offset + visible_items; 
                 i++) {
                
                int y = list_rect.top + (i - data->scroll_offset) * ITEM_HEIGHT;
                rect_t item_rect = {list_rect.left, y, list_rect.right, ITEM_HEIGHT};
                
                if (i == data->selected_index) {
                    draw_rect(&draw, &item_rect, 0xFF3399FF);
                }
                
                char text[512];
                snprintf(text, sizeof(text), "%s %s", 
                         data->entries[i].is_dir ? "[DIR] " : "[FILE]", 
                         data->entries[i].name);
                
                point_t pos = {item_rect.left + 8, item_rect.top + 6};
                uint32_t text_color = (i == data->selected_index) ? 0xFFFFFFFF : 0xFF000000;
                draw_string(&draw, data->font, &pos, text_color, text, strlen(text));
            }
            
            // 绘制按钮
            rect_t up_rect = {10, WINDOW_HEIGHT - 35, 50, 25};
            draw_rect(&draw, &up_rect, 0xFFCCCCCC);
            
            rect_t ref_rect = {70, WINDOW_HEIGHT - 35, 60, 25};
            draw_rect(&draw, &ref_rect, 0xFFCCCCCC);
            
            if (data->font != NULL) {
                point_t up_pos = {28, WINDOW_HEIGHT - 25};
                draw_string(&draw, data->font, &up_pos, 0xFF000000, "Up", 2);
                
                point_t ref_pos = {85, WINDOW_HEIGHT - 25};
                draw_string(&draw, data->font, &ref_pos, 0xFF000000, "Ref", 3);
            }
            
            element_draw_end(elem, &draw);
            break;
        }
        
        case EVENT_KBD: {
            file_manager_data_t* data = element_get_private(elem);
            if (data == NULL) break;
            
            if (event->kbd.type != KBD_PRESS) break;
            
            if (event->kbd.code == KBD_UP && data->selected_index > 0) {
                data->selected_index--;
                if (data->selected_index < data->scroll_offset) {
                    data->scroll_offset = data->selected_index;
                }
                element_redraw(elem, true);
            }
            else if (event->kbd.code == KBD_DOWN && data->selected_index + 1 < data->entry_count) {
                data->selected_index++;
                int visible = (WINDOW_HEIGHT - 90) / ITEM_HEIGHT;
                if (data->selected_index >= data->scroll_offset + visible) {
                    data->scroll_offset = data->selected_index - visible + 1;
                }
                element_redraw(elem, true);
            }
            else if (event->kbd.code == KBD_ENTER && data->selected_index < data->entry_count) {
                if (data->entries[data->selected_index].is_dir) {
                    load_directory(data, data->entries[data->selected_index].path);
                    element_redraw(elem, true);
                }
            }
            else if (event->kbd.code == KBD_BACKSPACE) {
                char* last_slash = strrchr(data->current_path, '/');
                if (last_slash != NULL && last_slash != data->current_path) {
                    *last_slash = '\0';
                    load_directory(data, data->current_path);
                } else if (last_slash == data->current_path) {
                    load_directory(data, "/");
                }
                element_redraw(elem, true);
            }
            break;
        }
        
        case EVENT_LIB_QUIT: {
            display_disconnect(window_get_display(win));
            break;
        }
    }
    return 0;
}

int main(void)
{
    printf("File Manager: Starting application...\n");
    
    display_t* disp = display_new();
    if (disp == NULL) {
        printf("File Manager: Failed to create display\n");
        return EXIT_FAILURE;
    }
    printf("File Manager: Display created successfully\n");
    
    rect_t rect = RECT_INIT_DIM(100, 100, WINDOW_WIDTH, WINDOW_HEIGHT);
    window_t* win = window_new(disp, "File Manager", &rect, 
                               SURFACE_WINDOW, WINDOW_DECO, 
                               procedure, NULL);
    if (win == NULL) {
        printf("File Manager: Failed to create window\n");
        display_free(disp);
        return EXIT_FAILURE;
    }
    printf("File Manager: Window created successfully\n");
    
    if (window_set_visible(win, true) == ERR) {
        printf("File Manager: Failed to make window visible\n");
        window_free(win);
        display_free(disp);
        return EXIT_FAILURE;
    }
    printf("File Manager: Window is now visible\n");
    
    event_t event = {0};
    while (display_next(disp, &event, CLOCKS_NEVER) != ERR) {
        display_dispatch(disp, &event);
    }
    
    printf("File Manager: Exiting...\n");
    
    window_free(win);
    display_free(disp);
    
    return EXIT_SUCCESS;
}