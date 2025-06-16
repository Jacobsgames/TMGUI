//HEADER FILE .h
#ifndef TMGUI_H
#define TMGUI_H

#include <stddef.h>
#include "raylib.h"

#define MIN -1
#define MAX -2
#define ABS(value) (value)
// ====== Config Constants ======
#define MAX_LAYOUT_STACK 16

// ====== Structs ======

// Grid types
typedef struct gridrect {
    int x, y;
    int w, h;
} gridrect;

typedef struct {
    int x; // column
    int y; // row
} atlaspos;

// Layout enums
typedef enum {
    LAYOUT_NONE,
    LAYOUT_VBOX,
    LAYOUT_HBOX
} LayoutMode;

// ====== Macros ======
#define TILE(x, y) ((atlaspos){ (x), (y) })
#define TILE_A (TILE(13, 13))
#define TILE_B (TILE(14, 15))
#define TILE_C (TILE(15, 15))
#define R(x, y, w, h) ((gridrect){ (x), (y), (w), (h) })
#define AUTO ((gridrect){ -1, -1, 0, 1 })


// ====== Style ======
typedef struct {
    LayoutMode mode;
    gridrect cursor;
    gridrect origin;
    gridrect requested; // <- stores original layout request (incl. sizing hints)
} LayoutContext;

typedef struct {
    Color background;
    Color foreground;
    Color border;
    int border_width;
} tm_style;

#define STYLE_TMGUI (tm_style){ .background = DARKGRAY, .foreground = LIGHTGRAY, .border = GRAY, .border_width = 1 }
#define STYLE_BW    (tm_style){ .background = BLACK, .foreground = WHITE, .border = WHITE, .border_width = 1 }

// ====== Layout System ======
void tm_push_layout(LayoutMode mode, gridrect anchor, gridrect requested);
void tm_pop_layout(void);

int tm_resolve_axis(int value, int fallback, int available);

gridrect tm_next_cell(int w, int h);

// ====== API ======
void tmgui_init(void);
void tmgui_shutdown(void);

// Style / font
void tm_set_font(Font font);
void tm_set_style(tm_style style);

// Elements
void tm_label(const char *text, int x, int y, Color color);
void tm_rect(gridrect r);
void tm_button(const char *label, gridrect r);
void tm_drawtile(int x, int y, atlaspos tile);

// Layout
void tm_vbox(gridrect r);
void tm_hbox(gridrect r);
void tm_end_box(void);

#endif // TMGUI_H
