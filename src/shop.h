
#ifndef SHOP_ONCE
#define SHOP_ONCE

#ifndef CSV_SQL
    // this is kinda just here for reference, `CSV_SQL` should be defined in `shop.c` before `#include "shop.h"` 
    typedef struct {
        char **columns,**cells;
        int width,height;
        char* error;
    } SQL_Result;

    // We took the bounds checker in and gave him a cartel execution
    #define CELL(r, x, y) ((r)->cells[(y)*(r)->width+(x)])
#endif

typedef struct {
    size_t count,capacity;
    char* str;
} SQL_Template;

#define NEW_SQL (SQL_Template){0,8,NULL}

#define APPEND_SQL(t, ...) do {\
    int needed = snprintf(NULL, 0, __VA_ARGS__);\
    if (t.count + (size_t)needed + 1 > t.capacity) {\
        while (t.count + (size_t)needed + 1 > t.capacity) {\
            t.capacity *= 2;\
        }\
        t.str = realloc(t.str, t.capacity);\
    }\
    t.count += snprintf(t.str + t.count, t.capacity - t.count, __VA_ARGS__);\
} while (0)

// not technically individual free because dynamic arrays are the PRECURSOR to arenas
#define NUKE_SQL(t) free(t.str)

typedef struct {
    char **names, **types;
    int count, capacity;
    Arena alloc;
} Schema_List;

typedef struct {
    char* data;
    size_t count,capacity;
    bool dirty;
    Arena alloc;
} Text_Editor;

typedef struct {
    Text_Editor* current_ed;
    float split_h,split_v;
    sqlite3* db;
    SQL_Result prev_result; 
    Schema_List schema;
    bool active;
    char csv_path_buf[500];

    Arena alloc;
    Arena temp;
} Admin_Panel;

typedef enum {
    LOAD_SCREEN,
    HOME_SCREEN,
    DISPLAY_SCREEN,
    // ACCOUNT_SCREEN,
    // CHECKOUT_SCREEN,
    // IDK??
} Screen;

typedef struct {
    Texture2D texture;
    Screen transition;
    const char* sql;
} Home_Button;

typedef struct {
    int id;
    char name[100];
    char description[500];
    float price;
    int stock;
} Item;

typedef struct {
    Item* items;
    int count, capacity;
    Arena alloc;
} Item_List;

typedef struct {
    bool is_model;
    float scale;
    union {
        Texture2D texture;
        Model model;
    };
} Item_Resource;

typedef Ht(int, Item_Resource) Item_Resource_Table;

typedef struct Shop {
    Admin_Panel admin;
    Item_Resource_Table item_resources;

    // HOME
    Home_Button* home_buttons;
    int home_button_count;
    Item_List featured;
    float top_row_scroll, top_row_scroll_target;
    float bottom_row_scroll, bottom_row_scroll_target;

    // DISPLAY
    Item_List display_items;
    float scroll, scroll_target;

    // CORE
    Camera3D camera;
    RenderTexture2D render_target;
    Shader shader;
    int time_loc; // cache this for perf
    Screen screen;
    bool transitioning;
    float transition_alpha;
    bool fading_out;
    Screen transition_target;
} Shop;

void strip_file_name(char *path);

void shop_render_pass(Shop* shop);
void ui_render_pass(Shop* shop);
void screen_swap(Shop* shop, Screen screen);
Vector2 mouse_pos_in_shop(Shop* shop);
void update_carousel(float* scroll, float* target, int count, float spacing);
Item_List get_items_by_name(Shop* shop, const char** names, int name_count);
void reset_item_list(Item_List* list);

bool init_shop(Shop *shop);
void update_shop(Shop *shop);
void draw_shop(Shop *shop);

#endif
