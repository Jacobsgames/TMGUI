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

static inline Rectangle gridrect_to_pixelrect(gridrect area) { // Converts a gridrect to a pixel-space Rectangle
    return (Rectangle){
        area.x * cell_w,
        area.y * cell_h,
        area.w * cell_w,
        area.h * cell_h
    };
}



static inline gridrect gridrect_offset(gridrect area, int x_off, int y_off) { //returns a copy of a grid rect, offset by input
    return (gridrect){ area.x + x_off, area.y + y_off, area.w, area.h };
}

// Relative offset of a CELL from a parent rect
static inline gridrect offset_cell(gridrect parent, int dx, int dy) {
    return CELL(parent.x + dx, parent.y + dy);
}

static inline bool gridrect_valid(gridrect area) { //checks if a gridrect has valid size (eg above 0 x,y)
    return (area.w > 0 && area.h > 0);
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
static inline bool rect_contains_cell(gridrect area, gridrect cell) {
    return (cell.x >= area.x && cell.x < area.x + area.w &&
            cell.y >= area.y && cell.y < area.y + area.h &&
            cell.w == 1 && cell.h == 1);
}

// Get center CELL of a rect
static inline gridrect rect_center_cell(gridrect area) {
    return CELL(area.x + area.w / 2, area.y + area.h / 2);
}

static gridrect get_area_and_txtpos(const char *text, gridrect area, gridrect *out_txtpos) {
    // Calculate text width in grid cells (optimized for monospaced fonts)
    int txt_w = strlen(text);

    // Determine widget's actual width (w):
    // 1. Use explicit width if provided (r.w > 0)
    // 2. Else, stretch to container width if in VBOX layout and container_w > 0
    // 3. Else, use text's natural width
    int w = (area.w > 0) ? area.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : txt_w);

    // Determine widget's actual height (h):
    // 1. Use explicit height if provided (r.h > 0)
    // 2. Else, default to 1 grid cell high for text
    int h = (area.h > 0) ? area.h : 1;

    // Determine widget's final position and size (final):
    // 1. If auto-positioning requested (r.x/y < 0), get next available cell from layout system
    // 2. Else, use explicit position from r combined with calculated w, h
    gridrect final = (area.x < 0 && area.y < 0) ? tm_next_cell(w, h) : (gridrect){ area.x, area.y, w, h };

    // Calculate and store the aligned starting grid cell for the text within 'final' rect
    *out_txtpos = tm_align_text_pos(final, txt_w, 1);

    // Return the calculated overall bounding rectangle for the widget
    return final;
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

inline void tm_draw_fill_rect(gridrect area, Color color) {
    Rectangle px = gridrect_to_pixelrect(area);
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
void tm_text(const char *text, gridrect area) {
    gridrect txtpos;
    (void)get_area_and_txtpos(text, area, &txtpos);
    tm_draw_text(text, txtpos, current_theme.text.foreground, current_theme.text.background);
}

void tm_label(const char *text, gridrect area) {
    gridrect txtpos;
    // HERE: final_label_area receives the returned value from the helper
    gridrect final_label_area = get_area_and_txtpos(text, area, &txtpos);
    // AND HERE: final_label_area is USED to draw the full background
    tm_draw_fill_rect(final_label_area, current_theme.label.background);
    tm_draw_text(text, txtpos, current_theme.label.foreground, current_theme.label.background);
}


/*bool tm_button(const char *label, gridrect r) {
    Font use_font = get_active_font();
    Vector2 txt_px = MeasureTextEx(use_font, label, cell_h, 0);
    int txt_w = pixels_to_grid_x(txt_px.x);

    int w = (r.w > 0) ? r.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : txt_w);
    int h = (r.h > 0) ? r.h : 1;

    gridrect final = (r.x < 0 && r.y < 0) ? tm_next_cell(w, h) : (gridrect){ r.x, r.y, w, h };

    gridrect text_cell_aligned = tm_align_text_pos(final, txt_w, 1); // This now returns a CELL gridrect

    tm_draw_text(label, text_cell_aligned, current_theme.label.foreground, current_theme.label.background);
}*/

gridrect tm_panel(gridrect area) {
    // compute final rect as usual
    int w = (area.w > 0) ? area.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : 1);
    int h = (area.h > 0) ? area.h : 1;
    gridrect final;

    if (area.x < 0 && area.y < 0) {
        final = tm_next_cell(w, h);
    } else {
        final = (gridrect){ area.x, area.y, w, h };
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

void tm_vbox(gridrect area) {
    gui_context.mode = LAYOUT_VBOX;
    gui_context.cursor_x = (area.x < 0 ? 0 : area.x);
    gui_context.cursor_y = (area.y < 0 ? 0 : area.y);
    gui_context.container_w = area.w; // ← Set container width
}

void tm_hbox(gridrect area) {
    gui_context.mode = LAYOUT_HBOX;
    gui_context.cursor_x = (area.x < 0 ? 0 : area.x);
    gui_context.cursor_y = (area.y < 0 ? 0 : area.y);
    gui_context.container_h = area.h; // ← Set container width
}



// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---
// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---
// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---// --- Main ---

// Test font declarations (as per your request)
Font customfont0;
Font customfont1;
Font customfont2;

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int cw = 8, ch = 8, gw = 80, gh = 47; // Full 80x47 grid screen
    InitWindow(gw * cw * 2, gh * ch * 2, "TMGUI Dungeon HUD (Revised)");
    tmgui_init(cw, ch);
    SetTargetFPS(60);

    // Load custom fonts (as per your request)
    customfont0 = LoadFontEx("C:/Code/tmgui/fonts/URSA.ttf", cell_h, NULL, 0);
    customfont1 = LoadFontEx("C:/Code/tmgui/fonts/DUNGEONMODE.ttf", cell_h, NULL, 0);
    customfont2 = LoadFontEx("C:/Code/tmgui/fonts/KITCHENSINK.ttf", cell_h, NULL, 0);
    SetTextureFilter(customfont0.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(customfont1.texture, TEXTURE_FILTER_POINT);
    SetTextureFilter(customfont2.texture, TEXTURE_FILTER_POINT);

    tm_canvas canvas = tm_canvas_init(gw, gh, false);

    // Set a custom theme for the main UI
    tm_set_theme(&THEME_GREEN);
    tm_set_font(&customfont0); // Use customfont0 as the primary UI font


    while (!WindowShouldClose()) {
    tm_canvas_begin(&canvas);
    ClearBackground(BLACK); // A darker background to highlight UI elements


    tm_set_spacing(1);
    ALIGN(LEFT, TOP);
// --- Inside your main game loop, within tm_canvas_begin(&canvas) and tm_canvas_end(&canvas) ---

// Assuming tm_set_spacing(1); and ALIGN(LEFT, TOP); are set at the start of your main loop iteration

// --- Panel Frame A and B (Visual boundaries for Vbox A & B) ---
tm_panel(RECT(0,0,13,45)); // list frame A
tm_panel(RECT(12,0,13,45)); // list frame B

// --- VBOX A: List of Labels and Text (your original example) ---
tm_vbox(RECT(1,1,11,45)); // This sets gui_context for elements in this Vbox
    
    ALIGN(LEFT,CENTER);
    tm_text("TEXT1",SIZE(11,1));

    ALIGN(CENTER,CENTER);
    tm_label("LABEL A",SIZE(11,3));
    tm_label("LABEL B",SIZE(11,3));
    tm_label("LABEL C",SIZE(11,3));
    tm_label("LABEL D",SIZE(11,3));

    ALIGN(LEFT,CENTER);
    tm_text("TEXT2",SIZE(11,1));

    ALIGN(CENTER,CENTER);
    tm_label("LABEL E",SIZE(11,3));
    tm_label("LABEL F",SIZE(11,3));
    tm_label("LABEL G",SIZE(11,3));
    tm_label("LABEL H",SIZE(11,3));

// --- VBOX B: Panels with children (your expanded weirdos list) ---
// This call *overwrites* gui_context, ending Vbox A's context and starting Vbox B's
tm_vbox(RECT(13,1,14,45)); // start Vbox B. Increased width to 14 for content.
    
    ALIGN(LEFT,TOP); // Alignment for elements *within* Vbox B

    // Character 1: Gorgon
    gridrect p0 = tm_panel(SIZE(14,5)); // Panel 0: Auto-positioned within Vbox B, full width of 14, height 5
    tm_label("Gorgon",RELRECT(p0,0,0,8,1)); // Name label, explicit 8x1 size at 0,0 offset
    tm_text("PWR: 6",OFFSET(p0,1,1)); // Power text, auto-sized, at 1,1 offset
    tm_text("SKI: 3",OFFSET(p0,1,2)); // Skill text, auto-sized, at 1,2 offset
    tm_text(">GobSmack",OFFSET(p0,1,3)); // Ability text, auto-sized, at 1,3 offset


    // Character 2: Velbort
    gridrect p1 = tm_panel(SIZE(14,5)); // Panel 1: Auto-positioned within Vbox B
    tm_label("Velbort",RELRECT(p1,0,0,8,1));
    tm_text("PWR: 3",OFFSET(p1,1,1));
    tm_text("SKI: 4",OFFSET(p1,1,2));
    tm_text(">Drain Life",OFFSET(p1,1,3));


    // Character 3: Glork
    gridrect p2 = tm_panel(SIZE(14,5)); // Panel 2: Auto-positioned within Vbox B
    tm_label("Glork",RELRECT(p2,0,0,8,1));
    tm_text("PWR: 8",OFFSET(p2,1,1));
    tm_text("SKI: 1",OFFSET(p2,1,2));
    tm_text(">Smash",OFFSET(p2,1,3));


    // Character 4: Zarthus
    gridrect p3 = tm_panel(SIZE(14,5)); // Panel 3: Auto-positioned within Vbox B
    tm_label("Zarthus",RELRECT(p3,0,0,8,1));
    tm_text("PWR: 5",OFFSET(p3,1,1));
    tm_text("SKI: 7",OFFSET(p3,1,2));
    tm_text(">Teleport",OFFSET(p3,1,3));


    // Character 5: Flink
    gridrect p4 = tm_panel(SIZE(14,5)); // Panel 4: Auto-positioned within Vbox B
    tm_label("Flink",RELRECT(p4,0,0,8,1));
    tm_text("PWR: 2",OFFSET(p4,1,1));
    tm_text("SKI: 9",OFFSET(p4,1,2));
    tm_text(">Charm",OFFSET(p4,1,3));


    // Character 6: Xylo
    gridrect p5 = tm_panel(SIZE(14,5)); // Panel 5: Auto-positioned within Vbox B
    tm_label("Xylo",RELRECT(p5,0,0,8,1));
    tm_text("PWR: 7",OFFSET(p5,1,1));
    tm_text("SKI: 5",OFFSET(p5,1,2));
    tm_text(">Sonic Burst",OFFSET(p5,1,3));
    // No spacing after the last panel in the list

// (Rest of your main loop code, tm_canvas_end, BeginDrawing/EndDrawing, etc.)


    tm_canvas_end(&canvas);

        BeginDrawing();
        ClearBackground(BLACK); // Clear the actual window with black
        EndDrawing();
    }

    // Unload fonts
    UnloadFont(customfont0);
    UnloadFont(customfont1);
    UnloadFont(customfont2);
    tmgui_shutdown(); // Unloads current_font and glyph_atlas
    CloseWindow();
    return 0;
}