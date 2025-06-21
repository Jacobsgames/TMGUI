#ifndef TMGUI_H
#define TMGUI_H

#include "raylib.h"

// --- Grid Rect ---
typedef struct { int x, y, w, h; } gridrect;

typedef struct {
	int x, y;
} gridpos;

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


#define AUTO ((gridrect){ -1, -1, 0, 0 })
#define SIZE(w,h) ((gridrect){ -1, -1, (w), (h) })
#define POS(x,y) ((gridrect){ x, y, 0, 0 })  

#define RECT(x,y,w,h) ((gridrect){ x, y, w, h })
#define GPOS(x, y) ((gridpos){ (x), (y) })

#define TEXT(s, x, y, fg, bg) tm_draw_text((s), GPOS((x), (y)), (fg), (bg))
#define GLYPH(x, y, tile, fg, bg) tm_draw_glyph((gridpos){ x, y }, tile, fg, bg)


#define ALIGN(h, v)       do { tm_align_horizontal(ALIGN_##h); tm_align_vertical(ALIGN_##v); } while (0)
#define ALIGNH(h)         tm_align_horizontal(ALIGN_##h)
#define ALIGNV(v)         tm_align_vertical(ALIGN_##v)

#define VSPACE(h) tm_label("", SIZE(-1, h))  // vertical spacing in VBOX
#define HSPACE(w) tm_label("", SIZE(w, -1))  // horizontal spacing in HBOX

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
	Color background, foreground, border;
	int border_width;
} tm_rect_style;

typedef struct {
	tm_rect_style normal;
	tm_rect_style hover;
	tm_rect_style active;
} tm_button_style;

typedef struct {
    Color background;     // Fill color inside the rect
    Color light_edge;     // Top-left edge color
    Color dark_edge;      // Bottom-right edge color
    int border_width;     // Width of the bevel edges in pixels
} tm_bevel_style;

typedef struct {
	tm_rect_style base;
	tm_button_style button;
	Font font;
	tm_bevel_style bevel;
} tm_style;


// --- Default Style ---
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
			.border = GREEN,
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
	.font = { 0 }, // fallback
	.bevel = {
		.background = BLACK,
		.light_edge = GREEN,
		.dark_edge = BLACK,
		.border_width = 2
	}
};

// --- Grey Style ---
static const tm_style STYLE_OSGREY = {
	.base = (tm_rect_style){
		.background = DARKGRAY,
		.foreground = WHITE,
		.border = GRAY,
		.border_width = 1
	},
	.button = {
		.normal = (tm_rect_style){
			.background = GRAY,
			.foreground = WHITE,
			.border = DARKGRAY,
			.border_width = 1
		},
		.hover = (tm_rect_style){
			.background = LIGHTGRAY,
			.foreground = WHITE,
			.border = WHITE,
			.border_width = 1
		},
		.active = (tm_rect_style){
			.background = BLACK,
			.foreground = GRAY,
			.border = DARKGRAY,
			.border_width = 1
		}
	},
	.font = { 0 }, // fallback
	.bevel = {
		.background = GRAY,
		.light_edge = LIGHTGRAY,
		.dark_edge = DARKGRAY,
		.border_width = 1
	}
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
void tm_align_horizontal(align_mode mode);
void tm_align_vertical(align_mode mode);
void tm_set_spacing(int spacing);

// --- Primitives 'tm_draw' ---
void tm_draw_fill_cell(gridpos p, Color color);
void tm_draw_fill_rect(gridrect r, Color color);
void tm_draw_glyph(gridpos pos, atlaspos tile, Color fg, Color bg);
void tm_draw_panel(gridrect r, const panel_kit *kit, Color fg, Color bg);
void tm_draw_text(const char *text, gridpos pos, Color fg, Color bg);

void tm_draw_style_rect(gridrect r);
void tm_draw_style_rect_bevel(gridrect r);

// --- Elements 'tm_f' ---
void tm_label(const char *label, gridrect r);
void tm_label_rect_styled(const char *label, gridrect r, const tm_rect_style *style);
void tm_label_rect(const char *label, gridrect r);

bool tm_button(const char *label, gridrect recti);


// --- Mouse Input / Transform ---
void tm_update_transform(int scale, int offX, int offY);
Vector2 tm_mouse_grid(void);

// --- Canvas Abstraction ---
tm_canvas tm_canvas_init(int grid_w, int grid_h, bool transparent);
void tm_canvas_begin(tm_canvas *c);
void tm_canvas_end(tm_canvas *c);

#endif // TMGUI_H
