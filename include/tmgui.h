// tmgui.h
#ifndef TMGUI_H
#define TMGUI_H

#include "raylib.h"

// ——— Grid Rect & “AUTO” placement ———
typedef struct {
    int x, y, w, h;
} gridrect;

//  AUTO = “use current container’s origin+cursor; size computed by widget”
#define AUTO ((gridrect){ -1, -1, 0, 0 })
#define R(x,y,w,h) ((gridrect){ x, y, w, h })

// ——— Tile Atlas coords ———
typedef struct {
    int x, y;
} atlaspos;
#define TILE(x,y) ((atlaspos){ x, y })

#define TILE_A (TILE(13, 13)) // i.e. tile at column 2, row 4
#define TILE_B (TILE(14, 15))
#define TILE_C  (TILE(15, 15))

// ——— Styling ———
typedef struct {
    Color background;
    Color foreground;
    Color border;
    int border_width;
} tm_style;
// default dark-themed
#define STYLE_TMGUI (tm_style){ .background=DARKGRAY, .foreground=LIGHTGRAY, .border=GRAY, .border_width=1 }
// alternate b/w
#define STYLE_BW    (tm_style){ .background=BLACK,     .foreground=WHITE,     .border=WHITE, .border_width=1 }

// ——— Layout Context & Modes ———
typedef enum { LAYOUT_NONE, LAYOUT_HBOX, LAYOUT_VBOX } LayoutMode;
typedef struct {
    LayoutMode mode;   // none, hbox or vbox
    gridrect origin;   // container anchor in grid coords
    int cursor_x;      // how far we’ve flowed so far
    int cursor_y;
} LayoutContext;

// ——— Layout API ———
// push a new H/V-box, anchored at *anchor* (or AUTO), 
// then tm_end_box() pops it.
void tm_vbox(gridrect anchor);
void tm_hbox(gridrect anchor);
void tm_end_box(void);

// get the next grid cell of size (w,h) in current box
gridrect tm_next_cell(int w, int h);

// ——— Core API ———
void tmgui_init(void);
void tmgui_shutdown(void);
void tm_set_style(const tm_style *style);
void tm_set_font(Font font);

// draw primitives
void tm_rect(gridrect r);
void tm_label(const char *text, int x, int y, Color c);
bool tm_button(const char *label, gridrect r);

// tile-atlas drawing
void tm_drawtile(int x, int y, atlaspos tile);

#endif // TMGUI_H