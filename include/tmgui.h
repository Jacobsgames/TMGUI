#ifndef TMGUI_H
#define TMGUI_H
#include "raylib.h"

// --- Grid Rect ---
typedef struct { int x, y, w, h; } grect;

// --- Offscreen Canvas ---
typedef struct {
	RenderTexture2D target;
	int grid_w, grid_h;
	int scale;
	int offset_x, offset_y;
	bool transparent;
} tm_canvas;

// --- Layout Mode ---
typedef enum { LAYOUT_NONE, LAYOUT_HBOX, LAYOUT_VBOX } layout_mode;

typedef enum {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT,
	ALIGN_TOP,
	ALIGN_BOTTOM
} align_mode;

typedef struct {
    layout_mode mode;
    int cursor_x, cursor_y;
    int container_w, container_h;  // ← NEW: active layout width for propagation
} layout_context;

extern layout_context gui_context;

// --- Helpers ---

#define AUTO ((grect){ -1, -1, -1, -1 })
#define SIZE(w,h) ((grect){ -1, -1, (w), (h) })
#define POS(x,y) ((grect){ x, y, -1, -1 })

#define RECT(x,y,w,h) ((grect){ x, y, w, h })

// RELRECT: Explicitly position and size an element relative to a parent grect.
// (e.g., RELRECT(panel_area, 1, 1, 8, 3) for 8x3 rect at 1,1 offset from panel)
#define RELRECT(parent_r, dx, dy, width, height) \
    ((grect){ (parent_r).x + (dx), (parent_r).y + (dy), (width), (height) })

// RELPOS: Position an element relative to a parent grect, with auto-sizing.
// (e.g., RELPOS(panel_area, 1, 1) for auto-sized element at 1,1 offset from panel)
// The width/height will be determined by the elements's internal logic (e.g., strlen w, h 1 for tm_text/label).
#define RELPOS(parent_r, dx, dy) \
    ((grect){ (parent_r).x + (dx), (parent_r).y + (dy), -1, -1 }) // w= -1, h= -1 signals auto-size


#define CELL(x, y)             ((grect){ (x), (y), 1, 1 })    // formerly gridpos
#define OFFSET(r, dx, dy)      ((grect){ (r).x + (dx), (r).y + (dy), (r).w, (r).h }) // formerly RRECT or RPOS

// TEXT and GLYPH macros now expect a grect (CELL) for position
#define TEXT(s, cell_rect, fg, bg) tm_draw_text((s), (cell_rect), (fg), (bg))
#define GLYPH(cell_rect, tile, fg, bg) tm_draw_glyph((cell_rect), (tile), (fg), (bg))


#define ALIGN(h, v)        do { tm_align_horizontal(ALIGN_##h); tm_align_vertical(ALIGN_##v); } while (0)
#define ALIGNH(h)          tm_align_horizontal(ALIGN_##h)
#define ALIGNV(v)          tm_align_vertical(ALIGN_##v)

#define VSPACE(h) tm_text("", SIZE(-1, h)) // vertical spacing in VBOX
#define HSPACE(w) tm_text("", SIZE(w, -1)) // horizontal spacing in HBOX

// --- Tile Atlas ---
typedef struct { int x, y; } atlaspos;
#define TILE(x,y) ((atlaspos){ x, y })

typedef struct {
    atlaspos corner_tl;
    atlaspos corner_tr;
    atlaspos corner_bl;
    atlaspos corner_br;
    atlaspos edge_horizontal;
    atlaspos edge_vertical;
    atlaspos cap_left;
    atlaspos cap_right;
    atlaspos fill;
} panel_kit;

// --- Styles ---
typedef struct {
	Color foreground;
	Color background;
} tm_textstyle;

typedef struct {
    panel_kit kit;
    Color foreground;
    Color background;
} tm_panel_style;

typedef struct {
	tm_textstyle normal;
	tm_textstyle hover;
	tm_textstyle active;
} tm_button_style;

typedef struct {
    Font font;
    tm_textstyle text;
    tm_textstyle label;
    tm_panel_style panel;
    tm_button_style button;
} tm_theme;

static const panel_kit KIT_DEFAULT = {
    .corner_tl = (atlaspos){15, 15},
    .corner_tr = (atlaspos){15, 15},
    .corner_bl = (atlaspos){15, 15},
    .corner_br = (atlaspos){15, 15},
    .edge_horizontal = (atlaspos){4, 12},
    .edge_vertical   = (atlaspos){3, 11},
    .cap_left  = (atlaspos){4, 11},
    .cap_right = (atlaspos){3, 12},
    .fill = (atlaspos){0, 0}
};

static const tm_theme THEME_GREEN = {
    .font = { 0 },
    .text = { GREEN, BLACK },
    .label = { BLACK, GREEN },
    .button = {
        .normal = { WHITE, DARKGREEN },
        .hover  = { BLACK, GREEN },
        .active = { GREEN, BLACK }
    },
    .panel = {
        .kit = KIT_DEFAULT,
        .foreground = GREEN,
        .background = BLACK
    }
};

// --- Core Layout API ---
void tm_vbox(grect area);
void tm_hbox(grect area);
grect tm_next_cell(int w, int h);

// --- System Init ---
void tmgui_init(int cell_w, int cell_h);
void tmgui_shutdown(void);
void tm_set_theme(const tm_theme *theme);
void tm_set_font(Font *font);
void tm_align_horizontal(align_mode mode);
void tm_align_vertical(align_mode mode);
void tm_set_spacing(int spacing);
void tm_set_padding(int padding);

// --- Primitives 'tm_draw' ---
void tm_draw_fill_cell(grect cell, Color color);
void tm_draw_fill_rect(grect area, Color color);
void tm_draw_glyph(grect cell, atlaspos tile, Color fg, Color bg);
void tm_draw_panel(grect r);
void tm_draw_text(const char *text, grect cell, Color fg, Color bg);

// ALIGNMENT HELPER: Now returns a grect (CELL)
grect tm_align_text_pos(grect container, int text_width_in_cells, int text_height_in_cells);


// --- Elements 'tm_f' ---
grect tm_text(const char *text, grect area);
grect tm_label(const char *text, grect area);
grect tm_panel(grect area);
grect tm_label_panel(const char *text, grect area, int text_nudge_x);
grect tm_panel_titled (const char *text, grect area, int pad);

// --- Mouse Input / Transform ---
void tm_update_transform(int scale, int offX, int offY);
Vector2 tm_mouse_grid(void);

// --- Canvas Abstraction ---
tm_canvas tm_canvas_init(int grid_w, int grid_h, bool transparent);
void tm_canvas_begin(tm_canvas *c);
void tm_canvas_end(tm_canvas *c);

#endif // TMGUI_H