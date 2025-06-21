#include "tmgui.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>


static int cell_w, cell_h;

static tm_theme current_theme;
static Font current_font = {0};
static Font fallback_font = {0};
static int layout_spacing = 0;
static Texture2D glyph_atlas;
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



static inline gridrect gridrect_offset(gridrect r, int x_off, int y_off) { //returns a copy of a grid rect, offset by input
    return (gridrect){ r.x + x_off, r.y + y_off, r.w, r.h };
}

// Relative offset of a CELL from a parent rect
static inline gridrect offset_cell(gridrect parent, int dx, int dy) {
    return CELL(parent.x + dx, parent.y + dy);
}

static inline bool gridrect_valid(gridrect r) { //checks if a gridrect has valid size (eg above 0 x,y)
    return (r.w > 0 && r.h > 0);
}


static inline int pixels_to_grid_x(float pixels) { // Converts horizontal pixel distance to grid cells (X-axis)
    return (int)(pixels / (float)cell_w);
}


static inline int pixels_to_grid_y(float pixels) { // Converts vertical pixel distance to grid cells (Y-axis)
    return (int)(pixels / (float)cell_h);
}

// Replace gridpos_equal with rect position equality
static inline bool cell_equal(gridrect a, gridrect b) {
    return (a.x == b.x && a.y == b.y);
}

// Check if a CELL (gridrect with w=1,h=1) is inside a bigger gridrect
static inline bool rect_contains_cell(gridrect r, gridrect cell) {
    return (cell.x >= r.x && cell.x < r.x + r.w &&
            cell.y >= r.y && cell.y < r.y + r.h &&
            cell.w == 1 && cell.h == 1);
}

// Get center CELL of a rect
static inline gridrect rect_center_cell(gridrect r) {
    return CELL(r.x + r.w / 2, r.y + r.h / 2);
}


// --- DRAW HELPERS ---



// --- Init ---
void tmgui_init(int cell_width, int cell_height) {
    cell_w = cell_width;
    cell_h = cell_height;

    fallback_font = LoadFontEx("C:/Code/tmgui/fonts/FROGBLOCK.ttf", cell_h, NULL, 0);
    SetTextureFilter(fallback_font.texture, TEXTURE_FILTER_POINT);

    current_theme = THEME_GREEN;
    current_theme.font = fallback_font;

    glyph_atlas = LoadTexture("C:/Code/tmgui/glyphs/T_FROGBLOCK.png");
    SetTextureFilter(glyph_atlas, TEXTURE_FILTER_POINT);
}

void tmgui_shutdown(void) {
    UnloadFont(current_font);
    UnloadTexture(glyph_atlas);
}

// --- Config ---
void tm_set_theme(const tm_theme *theme) {
    if (theme) current_theme = *theme;
}

void tm_set_font(Font *font) {
    if (font && font->texture.id != 0) {
        current_font = *font;
    } else {
        current_font = current_theme.font;
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

// Returns a gridrect (CELL) for text starting point aligned inside rect
gridrect tm_align_text_pos(gridrect container, int text_width_in_cells, int text_height_in_cells) {
    int x = container.x; // padding now handled explicitly in caller
    int y = container.y;

    // Horizontal alignment
    if (h_align == ALIGN_CENTER)
        x = container.x + (container.w - text_width_in_cells) / 2;
    else if (h_align == ALIGN_RIGHT)
        x = container.x + container.w - text_width_in_cells;

    // Vertical alignment
    if (v_align == ALIGN_CENTER)
        y = container.y + (container.h - text_height_in_cells) / 2;
    else if (v_align == ALIGN_BOTTOM)
        y = container.y + container.h - text_height_in_cells;

    return CELL(x, y); // Return as a CELL gridrect
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
    if (current_theme.font.texture.id != 0) return current_theme.font;
    return fallback_font;
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
// --- Primitives ---//////////////////////////////////////////////

inline void tm_draw_fill_rect(gridrect r, Color color) {
    Rectangle px = gridrect_to_pixelrect(r);
    DrawRectangle(px.x, px.y, px.width, px.height, color);
}

inline void tm_draw_fill_cell(gridrect cell, Color color) {
    // Convert single-cell gridrect to pixel-space Rectangle
    Rectangle fill = gridrect_to_pixelrect((gridrect){ cell.x, cell.y, 1, 1 });
    // Draw filled rectangle using raylib helper
    DrawRectangleRec(fill, color);
}

void tm_draw_glyph(gridrect cell, atlaspos glyph, Color fg, Color bg) {
    Rectangle dest = (Rectangle){ cell.x * cell_w, cell.y * cell_h, cell_w, cell_h };
    Rectangle src = (Rectangle){ glyph.x * cell_w, glyph.y * cell_h, cell_w, cell_h };
    if (bg.a > 0) tm_draw_fill_cell(cell, bg);
    DrawTexturePro(glyph_atlas, src, dest, (Vector2){0, 0}, 0, fg);
}

void tm_draw_text(const char *text, gridrect cell, Color fg, Color bg) {
    Font font = get_active_font();

    for (int i = 0; text[i]; i++) {
        // Position of each character cell (offset horizontally)
        gridrect char_cell = { cell.x + i, cell.y, 1, 1 }; // Ensure w and h are 1 for character cells

        // Fill background for character cell
        tm_draw_fill_rect(char_cell, bg); // Use tm_draw_fill_rect as char_cell is a gridrect

        // Convert grid rect to pixel position
        Rectangle px = gridrect_to_pixelrect(char_cell);

        // Draw the character at pixel position
        DrawTextCodepoint(font, text[i], (Vector2){ px.x, px.y }, cell_h, fg);
    }
}

void tm_draw_panel(gridrect r) {
    if (!gridrect_valid(r)) return;

    tm_panel_style style = current_theme.panel;
    panel_kit *kit = &style.kit;
    Color fg = style.foreground;
    Color bg = style.background;

    // Fill panel background first
    tm_draw_fill_rect(r, bg);

    if (r.h == 1) {
        atlaspos left  = (kit->cap_left.x  >= 0) ? kit->cap_left  : kit->corner_tl;
        atlaspos right = (kit->cap_right.x >= 0) ? kit->cap_right : kit->corner_tr;

        tm_draw_glyph(gridrect_offset(r, 0, 0), left, fg, bg);
        for (int i = 1; i < r.w - 1; i++)
            tm_draw_glyph(gridrect_offset(r, i, 0), kit->edge_horizontal, fg, bg);
        if (r.w > 1)
            tm_draw_glyph(gridrect_offset(r, r.w - 1, 0), right, fg, bg);
        return;
    }

    // Draw corners
    tm_draw_glyph(gridrect_offset(r, 0, 0), kit->corner_tl, fg, bg);
    tm_draw_glyph(gridrect_offset(r, r.w - 1, 0), kit->corner_tr, fg, bg);
    tm_draw_glyph(gridrect_offset(r, 0, r.h - 1), kit->corner_bl, fg, bg);
    tm_draw_glyph(gridrect_offset(r, r.w - 1, r.h - 1), kit->corner_br, fg, bg);

    // Draw top and bottom edges
    for (int i = 1; i < r.w - 1; i++) {
        tm_draw_glyph(gridrect_offset(r, i, 0), kit->edge_horizontal, fg, bg);
        tm_draw_glyph(gridrect_offset(r, i, r.h - 1), kit->edge_horizontal, fg, bg);
    }

    // Draw left and right edges
    for (int j = 1; j < r.h - 1; j++) {
        tm_draw_glyph(gridrect_offset(r, 0, j), kit->edge_vertical, fg, bg);
        tm_draw_glyph(gridrect_offset(r, r.w - 1, j), kit->edge_vertical, fg, bg);
    }

    // Fill the interior
    for (int i = 1; i < r.w - 1; i++) {
        for (int j = 1; j < r.h - 1; j++) {
            tm_draw_glyph(gridrect_offset(r, i, j), kit->fill, fg, bg);
        }
    }
}




//
void tm_label(const char *label, gridrect r) {
    Font use_font = get_active_font();
    Vector2 txt_px = MeasureTextEx(use_font, label, cell_h, 0);
    int txt_w = pixels_to_grid_x(txt_px.x);

    int w = (r.w > 0) ? r.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : txt_w);
    int h = (r.h > 0) ? r.h : 1;

    gridrect final = (r.x < 0 && r.y < 0) ? tm_next_cell(w, h) : (gridrect){ r.x, r.y, w, h };

    gridrect text_cell_aligned = tm_align_text_pos(final, txt_w, 1); // This now returns a CELL gridrect

    tm_draw_text(label, text_cell_aligned, current_theme.label.foreground, current_theme.label.background);
}

gridrect tm_panel(gridrect r) {
    // compute final rect as usual
    int w = (r.w > 0) ? r.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : 1);
    int h = (r.h > 0) ? r.h : 1;
    gridrect final;

    if (r.x < 0 && r.y < 0) {
        final = tm_next_cell(w, h);
    } else {
        final = (gridrect){ r.x, r.y, w, h };
    }

    tm_draw_panel(final);
    return final;
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


    tm_canvas canvas = tm_canvas_init(gw, gh, false);

    while (!WindowShouldClose()) {

        tm_canvas_begin(&canvas);
        ALIGN(CENTER,CENTER);

        tm_vbox(RECT(0, 0, 40, 0));  // Start vertical layout at (0,0), 40 cells wide

            tm_label("== Settings Menu ==", AUTO);  // Top label in VBOX
            gridrect panel0 = tm_panel(SIZE(40, 6));  // Reserve space and draw the panel

                // Children inside the panel (relative to panel position)
                // Use CELL for single-cell positions within TEXT macro
                TEXT("Volume",     OFFSET(panel0, 2, 1), GREEN, BLACK);
                TEXT("[#####      ]", OFFSET(panel0, 2, 2), GREEN, BLACK);

            tm_label("== Footer Info ==", AUTO);  // Another label after panel
        ALIGN(LEFT,TOP);
            gridrect panel1 = tm_panel(SIZE(30, 10));
                tm_label("Inside:", OFFSET(panel1, 1, 1)); // 2 right, 1 down inside panel
                tm_label("Option A", OFFSET(panel1, 1, 2));
                tm_label("Option B", OFFSET(panel1, 1, 3));

                tm_draw_glyph(POS(40,40), (atlaspos){3, 3}, BLUE, BLANK);


        tm_canvas_end(&canvas);

        BeginDrawing();
        ClearBackground(BLUE);
        EndDrawing();
    }

    tmgui_shutdown();
    CloseWindow();
    return 0;
}