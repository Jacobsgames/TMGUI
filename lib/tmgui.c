#include "tmgui.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


static int cell_w, cell_h;

static tm_style current_style;
static Font current_font = {0};
static Font fallback_font = {0};
static int layout_spacing = 0;
static Texture2D tile_atlas;
static int canvas_scale, canvas_x, canvas_y;

layout_context gui_context = {0};

static align_mode h_align = ALIGN_LEFT;
static align_mode v_align = ALIGN_TOP;

// --- Transform ---
void tm_update_transform(int scale, int pos_x, int pos_y) {
    canvas_scale = scale;
    canvas_x = pos_x;
    canvas_y = pos_y;
}

Vector2 tm_mouse_grid(void) {
    Vector2 m = GetMousePosition();
    float pixel_rect = (m.x - canvas_x) / (float)canvas_scale;
    float py = (m.y - canvas_y) / (float)canvas_scale;
    return (Vector2){ pixel_rect / cell_w, py / cell_h };
}


// --- GRIDTOOLS ---

static inline Rectangle gridrect_to_pixelrect(gridrect r) { // Converts a gridrect to a pixel-space Rectangle
    return (Rectangle){
        r.x * cell_w,
        r.y * cell_h,
        r.w * cell_w,
        r.h * cell_h
    };
}

static inline gridpos gridrect_pos_offset(gridrect r, int x_off, int y_off) { //returns the POS of a grid rect, offset by input
    return (gridpos){ r.x + x_off, r.y + y_off };
}

static inline gridrect gridrect_offset(gridrect r, int x_off, int y_off) { //returns a copy of a grid rect, offset by input
    return (gridrect){ r.x + x_off, r.y + y_off, r.w, r.h };
}

static inline bool gridrect_valid(gridrect r) { //checks if a gridrect has valid size (eg above 0 x,y)
    return (r.w > 0 && r.h > 0);
}

static inline Vector2 gridpos_to_pixelpos(gridpos p) { // Converts a gridpos to a pixel-space Vector2
    return (Vector2){ p.x * cell_w, p.y * cell_h };
}


static inline int pixels_to_grid_x(float pixels) { // Converts horizontal pixel distance to grid cells (X-axis)
    return (int)(pixels / (float)cell_w);
}


static inline int pixels_to_grid_y(float pixels) { // Converts vertical pixel distance to grid cells (Y-axis)
    return (int)(pixels / (float)cell_h);
}

static inline bool gridpos_equal(gridpos a, gridpos b) { // checks equality between two grid positions
    return (a.x == b.x && a.y == b.y);
}

static inline bool gridrect_contains(gridrect r, gridpos p) { // check if a gridpos is inside a rect
    return (p.x >= r.x && p.x < r.x + r.w &&
            p.y >= r.y && p.y < r.y + r.h);
}

static inline gridpos gridrect_center(gridrect r) { //  get the center point of a rect
    return (gridpos){ r.x + r.w / 2, r.y + r.h / 2 };
}


// --- DRAW HELPERS ---



// --- Init ---
void tmgui_init(int cell_width, int cell_height) {
    cell_w = cell_width;
    cell_h = cell_height;

    fallback_font = LoadFontEx("C:/Code/tmgui/fonts/FROGBLOCK.ttf", cell_h, NULL, 0);
    SetTextureFilter(fallback_font.texture, TEXTURE_FILTER_POINT);

    current_style = STYLE_TMGUI;
    current_style.font = fallback_font;

    tile_atlas = LoadTexture("C:/Code/tmgui/tiles/T_FROGBLOCK.png");
    SetTextureFilter(tile_atlas, TEXTURE_FILTER_POINT);
}

void tmgui_shutdown(void) {
    UnloadFont(current_font);
    UnloadTexture(tile_atlas);
}

// --- Config ---
void tm_set_style(const tm_style *style) {
    if (style) current_style = *style;
}

void tm_set_font(Font *font) {
    if (font && font->texture.id != 0) {
        current_font = *font;
    } else {
        current_font = current_style.font;
    }
}

void tm_set_spacing(int spacing) {
    layout_spacing = spacing;
}

void tm_align_horizontal(align_mode mode) {
    h_align = mode;
}

void tm_align_vertical(align_mode mode) {
    v_align = mode;
}

// Returns a gridpos for text starting point aligned inside rect
gridpos tm_align_text_pos(gridrect container, int text_width_in_cells, int text_height_in_cells) {
    int x = container.x; // padding now handled explicitly in caller
    int y = container.y;

    // Horizontal alignment
    if (h_align == ALIGN_CENTER)
        x = container.x + (container.w - text_width_in_cells) / 2;
    else if (h_align == ALIGN_RIGHT)
        x = container.x + container.w - text_width_in_cells - 1;

    // Vertical alignment
    if (v_align == ALIGN_CENTER)
        y = container.y + (container.h - text_height_in_cells) / 2;
    else if (v_align == ALIGN_BOTTOM)
        y = container.y + container.h - text_height_in_cells;

    return (gridpos){ x, y };
}


// --- Canvas ---
tm_canvas tm_canvas_init(int grid_w, int grid_h, bool transparent) {
    tm_canvas c = {0};
    c.grid_w = grid_w;
    c.grid_h = grid_h;
    c.transparent = transparent;
    c.target = LoadRenderTexture(grid_w * cell_w, grid_h * cell_h);
    SetTextureFilter(c.target.texture, TEXTURE_FILTER_POINT);
    return c;
}

void tm_canvas_begin(tm_canvas *c) {
    BeginTextureMode(c->target);
    ClearBackground(c->transparent ? BLANK : BLACK);
}

void tm_canvas_end(tm_canvas *c) {
    EndTextureMode();
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int sx = sw / (c->grid_w * cell_w), sy = sh / (c->grid_h * cell_h);
    c->scale = (sx < sy ? sx : sy);
    if (c->scale < 1) c->scale = 1;

    int dw = c->grid_w * cell_w * c->scale;
    int dh = c->grid_h * cell_h * c->scale;
    c->offset_x = (sw - dw) / 2;
    c->offset_y = (sh - dh) / 2;

    tm_update_transform(c->scale, c->offset_x, c->offset_y);

    DrawTexturePro(c->target.texture,
        (Rectangle){ 0, 0, c->grid_w * cell_w, -c->grid_h * cell_h },
        (Rectangle){ c->offset_x, c->offset_y, dw, dh },
        (Vector2){ 0, 0 }, 0, WHITE);
}

static Font get_active_font(void) {
    if (current_font.texture.id != 0) return current_font;
    if (current_style.font.texture.id != 0) return current_style.font;
    return fallback_font;
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
// --- Primitives ---//////////////////////////////////////////////

inline void tm_draw_fill_rect(gridrect r, Color color) {
    Rectangle px = gridrect_to_pixelrect(r);
    DrawRectangle(px.x, px.y, px.width, px.height, color);
}

inline void tm_draw_fill_cell(gridpos p, Color color) {
    // Convert single-cell gridrect to pixel-space Rectangle
    Rectangle cell = gridrect_to_pixelrect((gridrect){ p.x, p.y, 1, 1 });
    // Draw filled rectangle using raylib helper
    DrawRectangleRec(cell, color);
}

void tm_draw_style_rect(gridrect r) {
    tm_draw_fill_rect(r, current_style.base.background);
    if (current_style.base.border_width > 0)
        DrawRectangleLinesEx(gridrect_to_pixelrect(r), current_style.base.border_width, current_style.base.border);
}

void tm_draw_bevel_rect(gridrect r) {
    tm_bevel_style s = current_style.bevel;
    Rectangle px = gridrect_to_pixelrect(r);

    // Fill background
    tm_draw_fill_rect(r, s.background);

    // Top + left = light
    DrawRectangle(px.x, px.y, px.width, s.border_width, s.light_edge);
    DrawRectangle(px.x, px.y, s.border_width, px.height, s.light_edge);

    // Bottom + right = dark
    DrawRectangle(px.x, px.y + px.height - s.border_width, px.width, s.border_width, s.dark_edge);
    DrawRectangle(px.x + px.width - s.border_width, px.y, s.border_width, px.height, s.dark_edge);
}

void tm_draw_glyph(gridpos pos, atlaspos tile, Color fg, Color bg) {
    Rectangle dest = (Rectangle){ pos.x * cell_w, pos.y * cell_h, cell_w, cell_h };
    Rectangle src = (Rectangle){ tile.x * cell_w, tile.y * cell_h, cell_w, cell_h };
    if (bg.a > 0) tm_draw_fill_cell(pos, bg);
    DrawTexturePro(tile_atlas, src, dest, (Vector2){0, 0}, 0, fg);
}

void tm_draw_text(const char *text, gridpos pos, Color fg, Color bg) {
    Font font = get_active_font();
    for (int i = 0; text[i]; i++) {
        gridpos p = { pos.x + i, pos.y };
        tm_draw_fill_cell(p, bg); // Fill background for this glyph cell
        Rectangle cell = gridrect_to_pixelrect((gridrect){ p.x, p.y, 1, 1 }); // Draw glyph at pixel position of cell
        DrawTextCodepoint(font, text[i], (Vector2){ cell.x, cell.y }, cell_h, fg); // raylib func for char draw (takes pixel pos)
    }
}

void tm_draw_panel(gridrect r, const panel_kit *kit, Color fg, Color bg) {
    if (!gridrect_valid(r)) return;

    tm_draw_fill_rect(r, bg);

    if (r.h == 1) {
        atlaspos left  = (kit->cap_left.x  >= 0) ? kit->cap_left  : kit->corner_tl;
        atlaspos right = (kit->cap_right.x >= 0) ? kit->cap_right : kit->corner_tr;

        tm_draw_glyph(gridrect_pos_offset(r, 0, 0), left, fg, bg);
        for (int i = 1; i < r.w - 1; i++)
            tm_draw_glyph(gridrect_pos_offset(r, i, 0), kit->edge_horizontal, fg, bg);
        if (r.w > 1)
            tm_draw_glyph(gridrect_pos_offset(r, r.w - 1, 0), right, fg, bg);
        return;
    }

    tm_draw_glyph(gridrect_pos_offset(r, 0, 0), kit->corner_tl, fg, bg);
    tm_draw_glyph(gridrect_pos_offset(r, r.w - 1, 0), kit->corner_tr, fg, bg);
    tm_draw_glyph(gridrect_pos_offset(r, 0, r.h - 1), kit->corner_bl, fg, bg);
    tm_draw_glyph(gridrect_pos_offset(r, r.w - 1, r.h - 1), kit->corner_br, fg, bg);

    for (int i = 1; i < r.w - 1; i++) {
        tm_draw_glyph(gridrect_pos_offset(r, i, 0), kit->edge_horizontal, fg, bg);
        tm_draw_glyph(gridrect_pos_offset(r, i, r.h - 1), kit->edge_horizontal, fg, bg);
    }

    for (int j = 1; j < r.h - 1; j++) {
        tm_draw_glyph(gridrect_pos_offset(r, 0, j), kit->edge_vertical, fg, bg);
        tm_draw_glyph(gridrect_pos_offset(r, r.w - 1, j), kit->edge_vertical, fg, bg);
    }

    for (int i = 1; i < r.w - 1; i++) {
        for (int j = 1; j < r.h - 1; j++) {
            tm_draw_glyph(gridrect_pos_offset(r, i, j), kit->fill, fg, bg);
        }
    }
}



// Use your helper to align text inside the rect and draw it with bg fill.
void tm_label(const char *label, gridrect r) {
    Font use_font = get_active_font();
    Vector2 txt_px = MeasureTextEx(use_font, label, cell_h, 0);
    int txt_w = pixels_to_grid_x(txt_px.x);

    int w;
    if (r.w > 0) w = r.w;
    else if (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0) w = gui_context.container_w;
    else w = txt_w;

    int h = (r.h > 0) ? r.h : 1;

    // Resolve final rect position and size
    gridrect final = (r.x < 0 && r.y < 0) ? tm_next_cell(w, h) : (gridrect){ r.x, r.y, w, h };

    // Get aligned text position inside final rect
    gridpos text_pos = tm_align_text_pos(final, txt_w, 1);

    // Draw label text with background fill (from current style)
    tm_draw_text(label, text_pos, current_style.base.foreground, current_style.base.background);
}


void tm_label_rect_styled(const char *label, gridrect r, const tm_rect_style *style) {
    Font use_font = get_active_font();
    Vector2 txt_px = MeasureTextEx(use_font, label, cell_h, 0);
    int txt_w = pixels_to_grid_x(txt_px.x);

    int padding = 0;
    int w = (r.w > 0) ? r.w : txt_w + padding * 2;
    int h = (r.h > 0) ? r.h : 1;

    gridrect final = (r.x < 0 && r.y < 0)
        ? tm_next_cell(w, h)
        : (gridrect){ r.x, r.y, w, h };

    // Background + border
    tm_draw_fill_rect(final, style->background);
    if (style->border_width > 0)
        DrawRectangleLinesEx(gridrect_to_pixelrect(final), style->border_width, style->border);

    // Aligned text
    gridpos text_pos = tm_align_text_pos(final, txt_w, 1);
    tm_draw_text(label, text_pos, style->foreground, BLANK);
}

void tm_label_rect(const char *label, gridrect r) {
    tm_label_rect_styled(label, r, &current_style.base);
}

bool tm_button(const char *label, gridrect recti) {
    Font font = get_active_font();
    Vector2 txt_px = MeasureTextEx(font, label, cell_h, 0);
    int txt_w = pixels_to_grid_x(txt_px.x);

    int w = (recti.w > 0) ? recti.w : txt_w;
    int h = (recti.h > 0) ? recti.h : 1;

    gridrect final_rect = (recti.x < 0 && recti.y < 0)
        ? tm_next_cell(w, h)
        : (gridrect){ recti.x, recti.y, w, h };

    Vector2 mouse_grid = tm_mouse_grid();
    bool over = gridrect_contains(final_rect, (gridpos){ (int)mouse_grid.x, (int)mouse_grid.y });

    bool pressed = over && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool clicked = over && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    const tm_rect_style *style = &current_style.button.normal;
    if (pressed) style = &current_style.button.active;
    else if (over) style = &current_style.button.hover;

    tm_label_rect_styled(label, final_rect, style);

    return clicked;
}





///////////////////////////////////////////////////
// --- Layout ---/////////////////////////////////
gridrect tm_next_cell(int w, int h) {
    int x = gui_context.cursor_x;
    int y = gui_context.cursor_y;

    if (gui_context.mode == LAYOUT_HBOX) gui_context.cursor_x += w + layout_spacing;
    else if (gui_context.mode == LAYOUT_VBOX) gui_context.cursor_y += h + layout_spacing;

    return (gridrect){ x, y, w, h };
}

void tm_vbox(gridrect r) {
    gui_context.mode = LAYOUT_VBOX;
    gui_context.cursor_x = (r.x < 0 ? 0 : r.x);
    gui_context.cursor_y = (r.y < 0 ? 0 : r.y);
    gui_context.container_w = r.w; // ← Set container width
}

void tm_hbox(gridrect r) {
    gui_context.mode = LAYOUT_HBOX;
    gui_context.cursor_x = (r.x < 0 ? 0 : r.x);
    gui_context.cursor_y = (r.y < 0 ? 0 : r.y);
    gui_context.container_h = r.h; // ← Set container width
}



// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---
// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---
// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---
Font customfont0;
Font customfont1;
Font customfont2;



int main(void) {

    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int cw = 8, ch = 8, gw = 80, gh = 47;
    InitWindow(gw * cw * 2, gh * ch * 2, "TMGUI Test");
    tmgui_init(cw, ch);
    SetTargetFPS(60);

    customfont0 = LoadFontEx("C:/Code/tmgui/fonts/URSA.ttf", cell_h, NULL, 0);
    customfont1 = LoadFontEx("C:/Code/tmgui/fonts/DUNGEONMODE.ttf", cell_h, NULL, 0);
    customfont2 = LoadFontEx("C:/Code/tmgui/fonts/KITCHENSINK.ttf", cell_h, NULL, 0);
    SetTextureFilter(customfont0.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(customfont1.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(customfont2.texture, TEXTURE_FILTER_POINT);

        panel_kit test_panel_kit = {
        .corner_tl = (atlaspos){15, 15},
        .corner_tr = (atlaspos){15, 15},
        .corner_bl = (atlaspos){15, 15},
        .corner_br = (atlaspos){15, 15},
        .edge_horizontal = (atlaspos){4, 12},
        .edge_vertical = (atlaspos){3, 11},
        .cap_left = (atlaspos){4, 11},
        .cap_right = (atlaspos){3, 12},
        .fill = (atlaspos){0, 0}
    };

    tm_canvas canvas = tm_canvas_init(gw, gh, false);

    while (!WindowShouldClose()) {

        tm_canvas_begin(&canvas);
            tm_set_style(&STYLE_TMGUI);
            tm_set_font(&customfont0);
            tm_set_spacing(0);
            tm_hbox(RECT(0, 0, 80, 1));
            tm_button("FILE",AUTO);
            tm_button("VIEW",AUTO);
            tm_button("TOOLS",AUTO);
            tm_button("HELP",AUTO);
            TEXT("Welcome to TMGUI!", 30, 1, GREEN, BLANK);
            TEXT("Premier GUI for old-ass wretched shit", 30, 2, BLACK, GREEN);
            tm_draw_panel(RECT(1, 2, 22, 41),&test_panel_kit, GREEN, BLACK);

            tm_vbox(RECT(2, 3, 20, 0));
                tm_set_spacing(1);
                ALIGN(LEFT,CENTER);
                tm_label_rect("     VBOX START     ",AUTO);
                tm_button("OPTION1",SIZE(20, 3));
                tm_button("OPTION2",SIZE(20, 3));
                tm_button("OPTION3", SIZE(20, 3));
                tm_button("OPTION4",SIZE(20, 3));
                tm_label_rect("       HEADER       ",AUTO);
                tm_button("OPTION1",SIZE(20, 3));
                tm_button("OPTION1",SIZE(20, 3));
                tm_button("OPTION1",SIZE(20, 3));
                tm_label_rect("       HEADER       ",AUTO);
                if (tm_button("EXECUTE",SIZE(20, 5))) {TraceLog(LOG_INFO, "Play button clicked!");}
                tm_set_spacing(0);

                Color palette[] = { BLACK, GREEN };
                #define PALETTE_SIZE (sizeof(palette)/sizeof(palette[0]))
                tm_draw_panel(RECT(29, 9, 18, 18), &test_panel_kit, GREEN, BLACK);
                tm_draw_panel(RECT(32, 9, 12, 1), &test_panel_kit, GREEN, BLACK);
                TEXT("WOW GYPHS!", 33, 9, GREEN, BLACK);

                // Inside your drawing code, just before EndDrawing() or similar:
                for (int x = 0; x < 16; x++) {
                    for (int y = 0; y < 16; y++) {
                        Color fg = palette[rand() % PALETTE_SIZE];
                         Color bg;
                do {
                     bg = palette[rand() % PALETTE_SIZE];
                } while ((bg.r == fg.r) && (bg.g == fg.g) && (bg.b == fg.b));  // avoid same fg/bg

                tm_draw_glyph((gridpos){x+30,y+10}, (atlaspos){x,y}, fg, bg);
                    }
                }

                char fps_display[32];
                snprintf(fps_display, sizeof(fps_display), "FPS: %d", GetFPS());
                tm_draw_text(fps_display, (gridpos){ 70, 0 }, GREEN, BLANK);
 
        tm_canvas_end(&canvas);

        BeginDrawing();
        ClearBackground(BLUE);
        EndDrawing();
    }

    tmgui_shutdown();
    CloseWindow();
    return 0;
}
