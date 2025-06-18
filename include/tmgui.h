#ifndef TMGUI_H
#define TMGUI_H

#include "raylib.h"

// --- Grid Rect ---
typedef struct {
    int x, y, w, h;
} gridrect;

// --- Offscreen Canvas ---
typedef struct {
    RenderTexture2D target;
    int grid_w, grid_h;
    int scale;
    int offset_x, offset_y;
    bool transparent;
} ui_canvas;

// --- Layout Mode ---
typedef enum { LAYOUT_NONE, LAYOUT_HBOX, LAYOUT_VBOX } LayoutMode;

typedef enum { ALIGNMENT_LEFT, ALIGNMENT_CENTER, ALIGNMENT_RIGHT } tm_align_mode;

typedef struct {
    LayoutMode mode;
    int cursor_x;
    int cursor_y;
} layout_context;

extern layout_context gui_context;

// --- Helpers ---
#define AUTO ((gridrect){ -1, -1, 0, 0 })
#define SIZE(w,h) ((gridrect){ -1, -1, (w), (h) })
#define POS(x,y) ((gridrect){ x, y, 0, 0 })  
#define RECT(x,y,w,h) ((gridrect){ x, y, w, h })

#define ALIGN_LEFT   tm_align(ALIGNMENT_LEFT)
#define ALIGN_CENTER tm_align(ALIGNMENT_CENTER)
#define ALIGN_RIGHT  tm_align(ALIGNMENT_RIGHT)

// --- Tile Atlas ---
typedef struct { int x, y; } atlaspos;
#define TILE(x,y) ((atlaspos){ x, y })

// --- Styles ---
typedef struct {
    Color background, foreground, border;
    int border_width;
} tm_rect_style;

typedef struct {
    tm_rect_style normal;
    tm_rect_style hover;
    tm_rect_style active;
} tm_button_style;

typedef struct {
    tm_rect_style base;
    tm_button_style button;
    Font font;
} tm_style;


static const tm_style STYLE_TMGUI = {
    .base = (tm_rect_style){
            .background = GREEN,
            .foreground = BLACK,
            .border = GREEN,
            .border_width = 0
    },
    .button = {
        .normal = (tm_rect_style){
            .background = BLACK,
            .foreground = GREEN,
            .border = DARKGREEN,
            .border_width = 0
        },
        .hover = (tm_rect_style){
            .background = BLACK,
            .foreground = GREEN,
            .border = GREEN,
            .border_width = 1
        },
        .active = (tm_rect_style){
            .background = GREEN,
            .foreground = BLACK,
            .border = BLACK,
            .border_width = 1
        }
    },
    .font = { 0 } // Use fallback if not specified
};

#define STYLE_BASIC   (tm_rect_style){ DARKGRAY, LIGHTGRAY, GRAY, 1 }
#define STYLE_BW       (tm_rect_style){ BLACK,    WHITE,     WHITE, 1 }
#define STYLE_CONSOLE  (tm_rect_style){ GREEN,    BLACK,     BLACK, 1 }
#define STYLE_BTN_NORMAL (tm_rect_style){ GRAY,    WHITE, LIGHTGRAY, 1 }
#define STYLE_BTN_HOVER  (tm_rect_style){ LIGHTGRAY,WHITE, WHITE,     2 }
#define STYLE_BTN_ACTIVE (tm_rect_style){ DARKGRAY, WHITE, BLACK,     1 }

// --- Core Layout API ---
void tm_vbox(gridrect r);
void tm_hbox(gridrect r);
gridrect tm_next_cell(int w, int h);

// --- System Init ---
void tmgui_init(int cell_w, int cell_h);
void tmgui_shutdown(void);
void tm_set_style(const tm_style *style);
void tm_set_font(Font *font);
void tm_align(tm_align_mode mode);
void tm_set_spacing(int spacing);

// --- Primitives ---
void tm_rect(gridrect r);
void tm_text(const char *text, int x, int y, Color c);
void tm_label(const char *label, gridrect r);
void tm_drawtile(int x, int y, atlaspos tile);
bool tm_button(const char *label, gridrect recti);

// --- Mouse Input / Transform ---
void tm_update_transform(int scale, int offX, int offY);
Vector2 tm_mouse_grid(void);

// --- Canvas Abstraction ---
ui_canvas ui_canvas_init(int grid_w, int grid_h, bool transparent);
void ui_canvas_begin(ui_canvas *c);
void ui_canvas_end(ui_canvas *c);

#endif // TMGUI_H
