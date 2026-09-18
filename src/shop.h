
#ifndef SHOP_ONCE
#define SHOP_ONCE

#ifndef CSV_SQL
    // this is kinda just here for reference, `CSV_SQL` should be defined in `shop.c` before `#include "shop.h" 
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

// I am trying REALLY HARD right now to NOT write an entity system...
typedef struct {
    int id;
    char name[128];
    char description[512];
    float price;
    Texture2D texture;
    bool has_texture;
    Model model;
    Vector3 size;
    float scale;
} Item;

typedef enum {
    LOAD_SCREEN,
    HOME_SCREEN,
    DISPLAY_SCREEN,
    // ACCOUNT_SCREEN,
    // CHECKOUT_SCREEN,
    // IDK??
} Screen;

typedef struct Shop {
    Admin_Panel admin;
    Item* items;
    int item_count, item_cap;
    float scroll, scroll_target;
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

void shop_render_pass(Shop* shop);
void ui_render_pass(Shop* shop);
void screen_swap(Shop* shop, Screen screen);

bool init_shop(Shop *shop);
void update_shop(Shop *shop);
void draw_shop(Shop *shop);

#endif
