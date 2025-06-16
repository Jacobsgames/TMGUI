#ifndef TMGUI_H
#define TMGUI_H

#include "raylib.h"

// ——— Grid Rect ———
typedef struct {
    int x, y, w, h;
} gridrect;

// ——— Offscreen Canvas ———
typedef struct {
    RenderTexture2D target; // off‐screen texture
    int grid_w, grid_h;     // tile dimensions
    int scale;              // integer scale factor
    int offset_x, offset_y; // letterbox offsets
    bool transparent;       // clear to BLANK if true
} tm_canvas;

// Helpers to construct gridrects
#define AUTO ((gridrect){ -1, -1, 0, 0 })
#define R(x,y,w,h) ((gridrect){ x, y, w, h })

// ——— Tile Atlas ———
typedef struct { int x, y; } atlaspos;
#define TILE(x,y) ((atlaspos){ x, y })
#define TILE_A TILE(13,13)
#define TILE_B TILE(14,15)
#define TILE_C TILE(15,15)

// ——— Styles ———
typedef struct {
    Color background, foreground, border;
    int border_width;
} tm_style;

typedef enum { TM_ALIGN_LEFT, TM_ALIGN_CENTER, TM_ALIGN_RIGHT, TM_ALIGN_FILL } tm_alignment;

// default styles
#define STYLE_TMGUI    (tm_style){ DARKGRAY, LIGHTGRAY, GRAY, 1 }
#define STYLE_BW       (tm_style){ BLACK,    WHITE,     WHITE, 1 }
#define STYLE_CONSOLE  (tm_style){ GREEN,    BLACK,     BLACK, 1 }
#define STYLE_BTN_NORMAL (tm_style){ GRAY,    WHITE, LIGHTGRAY, 1 }
#define STYLE_BTN_HOVER  (tm_style){ LIGHTGRAY,WHITE, WHITE,     1 }
#define STYLE_BTN_ACTIVE (tm_style){ DARKGRAY, WHITE, BLACK,     2 }

// ——— Layout ———
typedef enum { LAYOUT_NONE, LAYOUT_HBOX, LAYOUT_VBOX } LayoutMode;
typedef struct {
    LayoutMode mode;
    gridrect   origin;    // anchor of this box
    int        cursor_x,
               cursor_y; // flow offset
    tm_alignment align;   // current alignment
    gridrect      requested; // requested box size
} LayoutContext;

// transformations & input
void tm_update_transform(int scale, int offX, int offY);
Vector2 tm_mouse_grid(void);

// flow API
void tm_vbox(gridrect anchor);
void tm_hbox(gridrect anchor);
void tm_end_box(void);
gridrect tm_next_cell(int w, int h);

// core API
void tmgui_init(void);
void tmgui_shutdown(void);
void tm_set_style(const tm_style *style);
void tm_set_font(Font font);

// primitives
void tm_rect(gridrect r);
void tm_label(const char *text, int x, int y, Color c);
bool tm_button(const char *label, gridrect r);
void tm_drawtile(int x, int y, atlaspos tile);

// canvas
tm_canvas tm_canvas_init(int grid_w, int grid_h, bool transparent);
void tm_canvas_begin(tm_canvas *c);
void tm_canvas_end(tm_canvas *c);

// test only
void tm_maketile(void);

#endif // TMGUI_H
