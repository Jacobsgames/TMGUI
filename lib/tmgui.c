#include "tmgui.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// - Canvas/Grid
static int cell_w, cell_h;
static int canvas_scale, canvas_x, canvas_y;

// - Layout
layout_context gui_context = {0};

static int layout_spacing = 0;
static int layout_padding = 0;
static align_mode h_align = ALIGN_LEFT;
static align_mode v_align = ALIGN_TOP;

// - Theme
static tm_theme current_theme;
static Font current_font = {0};
static Font fallback_font = {0};
static Texture2D glyph_atlas;

// - Devtools
static bool show_tilepicker = false;



// --- GRIDTOOLS ---------------------------------------------------------------------------------------

Vector2 tm_mouse_grid(void) {
    Vector2 m = GetMousePosition();
    float pixel_rect = (m.x - canvas_x) / (float)canvas_scale;
    float py = (m.y - canvas_y) / (float)canvas_scale;
    return (Vector2){ pixel_rect / cell_w, py / cell_h };
}

static inline Rectangle grect_to_pixelrect(grect area) { // Converts a grect to a pixel-space Rectangle
    return (Rectangle){
        area.x * cell_w,
        area.y * cell_h,
        area.w * cell_w,
        area.h * cell_h
    };
}

static inline grect grect_offset(grect area, int x_off, int y_off) { //returns a copy of a grid rect, offset by input
    return (grect){ area.x + x_off, area.y + y_off, area.w, area.h };
}


static inline grect offset_cell(grect parent, int dx, int dy) {
    return CELL(parent.x + dx, parent.y + dy); // Relative offset of a CELL from a parent rect
}

static inline bool grect_valid(grect area) { //checks if a grect has valid size (eg above 0 x,y)
    return (area.w > 0 && area.h > 0);
}

static inline int pixels_to_grid_x(float pixels) { // Converts horizontal pixel distance to grid cells (X-axis)
    return (int)(pixels / (float)cell_w);
}

static inline int pixels_to_grid_y(float pixels) { // Converts vertical pixel distance to grid cells (Y-axis)
    return (int)(pixels / (float)cell_h);   
}

static inline bool cell_equal(grect a, grect b) {
    return (a.x == b.x && a.y == b.y);
}

// Check if a CELL (grect with w=1,h=1) is inside a bigger grect
static inline bool rect_contains_cell(grect area, grect cell) {
    return (cell.x >= area.x && cell.x < area.x + area.w &&
            cell.y >= area.y && cell.y < area.y + area.h &&
            cell.w == 1 && cell.h == 1);
}

// Get center CELL of a rect
static inline grect rect_center_cell(grect area) {
    return CELL(area.x + area.w / 2, area.y + area.h / 2);
}

static grect get_area_and_txtpos(const char *text, grect area, grect *out_txtpos) { // THIS A BIT STINKY, ISSA BIG ONE. DOES A LOT
    
    int txt_w = strlen(text); // Calculate text width in grid cells (optimized for monospaced fonts)

    // Determine widget's actual width (w):
    int w = (area.w > 0) ? area.w : // If area.w positive, use that value.
            (area.w == -1 && // Else, if auto-width sentinel (-1) AND...
             area.x == -1 && area.y == -1 && // ...auto-positioned (both x,y are -1) AND...
             gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0) ? // ...VBOX mode.
            gui_context.container_w : // Then stretch to VBOX width.
            txt_w; // Else (auto-width but not stretching, or area.w is 0/other negative), use text's natural width.

    // Determine widget's actual height (h):
    int h = (area.h > 0) ? area.h : // Use explicit height if positive.
            ((area.h == -1) ? 1 : 1); // If auto-height sentinel (-1), use 1. Else (0 or other), also use 1.

    // Determine widget's final position and size (final):
    grect final;
    if (area.x == -1 && area.y == -1) { // If auto-positioning
        // Get the base position and advance the cursor for the next element
        grect base_pos = tm_next_cell(w, h); 
        
        int aligned_x = base_pos.x; // Start with the layout's default X
        // Apply horizontal alignment for the widget's starting position within the container
        if (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0) {
            if (h_align == ALIGN_CENTER) {
                aligned_x += (gui_context.container_w - w) / 2;
            } else if (h_align == ALIGN_RIGHT) {
                aligned_x += (gui_context.container_w - w);
            }
        }
        // Note: Vertical alignment for the widget's position is not usually.
        // done in VBOX as tm_next_cell handles vertical stacking directly.
        
        final = (grect){ aligned_x, base_pos.y, w, h }; // Construct final rect with aligned X
    } else { // If explicitly positioned (POS or RECT)
        final = (grect){ area.x, area.y, w, h };
    }

    
    *out_txtpos = tm_align_text_pos(final, txt_w, 1); // Calculate and store the aligned starting grid cell for the text within 'final' rect.

    
    return final; // Return the calculated overall bounding rectangle for the widget.
}


// --- TRANSFORM HELPERS --------------------------------------------------------------------------

void tm_update_transform(int scale, int pos_x, int pos_y) {
    canvas_scale = scale;
    canvas_x = pos_x;
    canvas_y = pos_y;
}


// --- INIT ---------------------------------------------------------------------------------------

void tmgui_init(int cell_width, int cell_height) {
    cell_w = cell_width;
    cell_h = cell_height;

    fallback_font = LoadFontEx("C:/Code/tmgui/fonts/BESCII.ttf", cell_h, NULL, 0);
    SetTextureFilter(fallback_font.texture, TEXTURE_FILTER_POINT);

    current_theme = THEME_GREEN;
    current_theme.font = fallback_font;

    glyph_atlas = LoadTexture("C:/Code/tmgui/glyphs/T_jpetscii.png");
    SetTextureFilter(glyph_atlas, TEXTURE_FILTER_POINT);
}

void tmgui_shutdown(void) {
    UnloadFont(current_font);
    UnloadTexture(glyph_atlas);
}

static Font get_active_font(void) {
    if (current_font.texture.id != 0) return current_font;
    if (current_theme.font.texture.id != 0) return current_theme.font;
    return fallback_font;
}


// --- LAYOUT SETTERS ---------------------------------------------------------------------------------------

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

void tm_set_padding(int padding) {
    layout_padding = padding;
}

void tm_align_horizontal(align_mode mode) {
    h_align = mode;
}

void tm_align_vertical(align_mode mode) {
    v_align = mode;
}

// Returns a grect (CELL) for text starting point aligned inside rect
grect tm_align_text_pos(grect container, int text_width_in_cells, int text_height_in_cells) {
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

    return CELL(x, y); // Return as a CELL grect
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


//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
// --- DRAW PRIMITIVES ------------------------------------------------------------------------------------

inline void tm_draw_fill_rect(grect area, Color color) {
    Rectangle px = grect_to_pixelrect(area);
    DrawRectangle(px.x, px.y, px.width, px.height, color);
}

inline void tm_draw_fill_cell(grect cell, Color color) {
    // Convert cell grect to pixel-space Rectangle
    Rectangle fill = grect_to_pixelrect((grect){ cell.x, cell.y, 1, 1 });
    // Draw filled rectangle using raylib helper
    DrawRectangleRec(fill, color);
}

void tm_draw_glyph(grect cell, atlaspos glyph, Color fg, Color bg) {
    Rectangle dest = (Rectangle){ cell.x * cell_w, cell.y * cell_h, cell_w, cell_h };
    Rectangle src = (Rectangle){ glyph.x * cell_w, glyph.y * cell_h, cell_w, cell_h };
    if (bg.a > 0) tm_draw_fill_cell(cell, bg);
    DrawTexturePro(glyph_atlas, src, dest, (Vector2){0, 0}, 0, fg);
}

void tm_draw_text(const char *text, grect cell, Color fg, Color bg) {
    Font font = get_active_font();

    for (int i = 0; text[i]; i++) {
        // Position of each character cell (offset horizontally)
        grect char_cell = { cell.x + i, cell.y, 1, 1 }; // Ensure w and h are 1 for character cells

        // Fill background for character cell
        tm_draw_fill_rect(char_cell, bg); // Use tm_draw_fill_rect as char_cell is a grect

        // Convert grid rect to pixel position
        Rectangle px = grect_to_pixelrect(char_cell);

        // Draw the character at pixel position
        DrawTextCodepoint(font, text[i], (Vector2){ px.x, px.y }, cell_h, fg);
    }
}

void tm_draw_panel(grect r) {
    if (!grect_valid(r)) return;

    tm_panel_style style = current_theme.panel;
    panel_kit *kit = &style.kit;
    Color fg = style.foreground;
    Color bg = style.background;

    // Fill panel background first
    tm_draw_fill_rect(r, bg);

    if (r.h == 1) {
        atlaspos left  = (kit->cap_l.x  >= 0) ? kit->cap_l  : kit->corner_tl;
        atlaspos right = (kit->cap_r.x >= 0) ? kit->cap_r : kit->corner_tr;

        tm_draw_glyph(grect_offset(r, 0, 0), left, fg, bg);
        for (int i = 1; i < r.w - 1; i++)
            tm_draw_glyph(grect_offset(r, i, 0), kit->strip, fg, bg);
        if (r.w > 1)
            tm_draw_glyph(grect_offset(r, r.w - 1, 0), right, fg, bg);
        return;
    }

    // Draw corners
    tm_draw_glyph(grect_offset(r, 0, 0), kit->corner_tl, fg, bg);
    tm_draw_glyph(grect_offset(r, r.w - 1, 0), kit->corner_tr, fg, bg);
    tm_draw_glyph(grect_offset(r, 0, r.h - 1), kit->corner_bl, fg, bg);
    tm_draw_glyph(grect_offset(r, r.w - 1, r.h - 1), kit->corner_br, fg, bg);

    // Draw top and bottom edges
    for (int i = 1; i < r.w - 1; i++) {
        tm_draw_glyph(grect_offset(r, i, 0), kit->edge_t, fg, bg);
        tm_draw_glyph(grect_offset(r, i, r.h - 1), kit->edge_b, fg, bg);
    }

    // Draw left and right edges
    for (int j = 1; j < r.h - 1; j++) {
        tm_draw_glyph(grect_offset(r, 0, j), kit->edge_l, fg, bg);
        tm_draw_glyph(grect_offset(r, r.w - 1, j), kit->edge_r, fg, bg);
    }

    // Fill the interior
    for (int i = 1; i < r.w - 1; i++) {
        for (int j = 1; j < r.h - 1; j++) {
            tm_draw_glyph(grect_offset(r, i, j), kit->fill, fg, bg);
        }
    }
}


// --- LAYOUT ELEMENTS ------------------------------------------------------------------------------------

grect tm_text(const char *text, grect area) {
    grect txtpos;
    grect final_area = get_area_and_txtpos(text, area, &txtpos);
    tm_draw_text(text, txtpos, current_theme.text.foreground, current_theme.text.background);
    return final_area;
}

grect tm_label(const char *text, grect area) {
    grect txtpos;
    // HERE: final_area receives the returned value from the helper
    grect final_area = get_area_and_txtpos(text, area, &txtpos);
    // AND HERE: final_area is USED to draw the full background
    tm_draw_fill_rect(final_area, current_theme.label.background);
    tm_draw_text(text, txtpos, current_theme.label.foreground, current_theme.label.background);
    return final_area;
}

grect tm_label_panel(const char *text, grect area, int padding) { 
    grect txtpos;
    grect final_area = get_area_and_txtpos(text, area, &txtpos); 
    tm_draw_panel(final_area); 
    
    int margin_pad = 0; // padding to add from margin IF in left/right

    if ( (h_align != ALIGN_CENTER) && (padding == -1) ) //if padding arg sentinel is -1, 'center pad'
        margin_pad = (final_area.w - strlen(text))/2; // calculates the margin padding for centering the text
    else 
        margin_pad = padding; // else use the manual padding

    // check which direction to 'pad' based on align mode
    switch (h_align) {
        case ALIGN_LEFT:  txtpos.x += margin_pad; break;
        case ALIGN_RIGHT: txtpos.x -= margin_pad; break;
        case ALIGN_CENTER:break;
        default:break;
    }

    //draw the text
    tm_draw_text(text, txtpos, current_theme.label.foreground, current_theme.label.background); 
    
    return final_area; 
}

/*bool tm_button(const char *label, grect r) { // WIP IGNORE
    Font use_font = get_active_font();
    Vector2 txt_px = MeasureTextEx(use_font, label, cell_h, 0);
    int txt_w = pixels_to_grid_x(txt_px.x);

    int w = (r.w > 0) ? r.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : txt_w);
    int h = (r.h > 0) ? r.h : 1;

    grect final = (r.x < 0 && r.y < 0) ? tm_next_cell(w, h) : (grect){ r.x, r.y, w, h };

    grect text_cell_aligned = tm_align_text_pos(final, txt_w, 1); // This now returns a CELL grect

    tm_draw_text(label, text_cell_aligned, current_theme.label.foreground, current_theme.label.background);
}*/

grect tm_panel(grect area) {
    // compute final rect as usual
    int w = (area.w > 0) ? area.w : (gui_context.mode == LAYOUT_VBOX && gui_context.container_w > 0 ? gui_context.container_w : 1);
    int h = (area.h > 0) ? area.h : 1;
    grect final;

    if (area.x < 0 && area.y < 0) {
        final = tm_next_cell(w, h);
    } else {
        final = (grect){ area.x, area.y, w, h };
    }

    tm_draw_panel(final);
    return final;
}

grect tm_panel_titled(const char *text, grect area, int pad) {
    grect final_area = tm_panel(area);

    // 1-line strip along top of panel, with padding
    grect title_area = {
        .x = final_area.x + pad,
        .y = final_area.y,
        .w = final_area.w - (2 * pad),
        .h = 1
    };

    grect textpos = tm_text(text, title_area);

    // Only draw caps if there's room
    if (pad >= 1) {
        grect lcap_cell = CELL(textpos.x - 1, textpos.y);
        grect rcap_cell = CELL(textpos.x + strlen(text), textpos.y);
        tm_draw_glyph(lcap_cell, current_theme.panel.kit.cap_l, current_theme.panel.foreground, current_theme.panel.background);
        tm_draw_glyph(rcap_cell, current_theme.panel.kit.cap_r, current_theme.panel.foreground, current_theme.panel.background);
    }

    grect content_area = {
        .x = final_area.x,
        .y = final_area.y + 1,
        .w = final_area.w,
        .h = final_area.h - 1
    };

    return content_area;
}


//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
// --- LAYOUT ---------------------------------------------------------------------------------------------
grect tm_next_cell(int w, int h) {
    int x = gui_context.cursor_x;
    int y = gui_context.cursor_y;

    if (gui_context.mode == LAYOUT_HBOX) gui_context.cursor_x += w + layout_spacing;
    else if (gui_context.mode == LAYOUT_VBOX) gui_context.cursor_y += h + layout_spacing;

    return (grect){ x, y, w, h };
}

void tm_vbox(grect area) {
    gui_context.mode = LAYOUT_VBOX;
    gui_context.cursor_x = (area.x < 0 ? 0 : area.x);
    gui_context.cursor_y = (area.y < 0 ? 0 : area.y);
    gui_context.container_w = area.w; // ← Set container width
}

void tm_hbox(grect area) {
    gui_context.mode = LAYOUT_HBOX;
    gui_context.cursor_x = (area.x < 0 ? 0 : area.x);
    gui_context.cursor_y = (area.y < 0 ? 0 : area.y);
    gui_context.container_h = area.h; // ← Set container width
}

// --- DEVTOOLS ---
void glyph_tool(void) {
    int atlas_columns = glyph_atlas.width / cell_w;
    int atlas_rows = glyph_atlas.height / cell_h;

    int grid_origin_x = 65;
    int grid_origin_y = 64;

    int glyph_draw_width = cell_w * 4;
    int glyph_draw_height = cell_h * 4;

    Vector2 mouse = GetMousePosition();
    int hovered_cell_x = (mouse.x - grid_origin_x) / glyph_draw_width;
    int hovered_cell_y = (mouse.y - grid_origin_y) / glyph_draw_height;

    // Background
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

    // Glyph Grid Bounds
    DrawRectangle(grid_origin_x - glyph_draw_width, grid_origin_y - glyph_draw_height,
                  (atlas_columns + 2) * glyph_draw_width, (atlas_rows + 2) * glyph_draw_height, BLACK);
    DrawRectangleLines(grid_origin_x - glyph_draw_width, grid_origin_y - glyph_draw_height,
                       (atlas_columns + 2) * glyph_draw_width, (atlas_rows + 2) * glyph_draw_height, GREEN);
    DrawTextEx(fallback_font, "GLYPH TOOL",
               (Vector2){ grid_origin_x - 32, grid_origin_y - 2 * glyph_draw_height },
               glyph_draw_height, 0, GREEN);

    // Draw all glyphs in grid
    for (int cell_y = 0; cell_y < atlas_rows; cell_y++) {
        for (int cell_x = 0; cell_x < atlas_columns; cell_x++) {
            Rectangle src = { cell_x * cell_w, cell_y * cell_h, cell_w, cell_h };
            Rectangle dst = { grid_origin_x + cell_x * glyph_draw_width,
                              grid_origin_y + cell_y * glyph_draw_height,
                              glyph_draw_width, glyph_draw_height };

            DrawTexturePro(glyph_atlas, src, dst, (Vector2){0}, 0, WHITE);
            DrawRectangleLines(dst.x, dst.y, dst.width, dst.height, DARKGREEN);

            if (cell_x == hovered_cell_x && cell_y == hovered_cell_y) {
                BeginBlendMode(BLEND_MULTIPLIED);
                DrawRectangle(dst.x, dst.y, dst.width, dst.height, GREEN);
                EndBlendMode();
            }
        }
    }

    // Draw hovered cell coordinates
    if (hovered_cell_x >= 0 && hovered_cell_x < atlas_columns &&
        hovered_cell_y >= 0 && hovered_cell_y < atlas_rows) {
        char buf[32];
        snprintf(buf, sizeof(buf), "[%d,%d]", hovered_cell_x, hovered_cell_y);
        DrawTextEx(fallback_font, buf,
                   (Vector2){ grid_origin_x + (atlas_columns - 5) * glyph_draw_width,
                              grid_origin_y + (atlas_rows + 1) * glyph_draw_height },
                   glyph_draw_height, 0, GREEN);
    }

    typedef struct { char text[64]; int x, y; bool glyph; } Log;
    static Log log[256];
    static int log_count = 0;
    static const char *part_names[12] = {
        "corner_tl", "corner_tr", "corner_bl", "corner_br",
        "edge_t", "edge_b", "edge_l", "edge_r",
        "fill", "cap_l", "cap_r", "strip"
    };
    static int selected_index = 0;
    static int preview_start_index = -1;

    // Left click to assign glyphs
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (selected_index == 0 && log_count < 255)
            strncpy(log[log_count++].text, ">>[New Set]--------------------------", 64);

        if (hovered_cell_x >= 0 && hovered_cell_x < atlas_columns &&
            hovered_cell_y >= 0 && hovered_cell_y < atlas_rows && selected_index < 12) {

            snprintf(log[log_count].text, 64, " .%s = (atlaspos){%d, %d},",
                     part_names[selected_index], hovered_cell_x, hovered_cell_y);

            ((atlaspos*)&current_theme.panel.kit)[selected_index] = (atlaspos){ hovered_cell_x, hovered_cell_y };

            log[log_count].x = hovered_cell_x;
            log[log_count].y = hovered_cell_y;
            log[log_count].glyph = true;
            printf("%s\n", log[log_count].text); fflush(stdout);
            if (log_count < 255) log_count++;
            selected_index++;
        }

        if (selected_index == 12) {
            preview_start_index = log_count - 12;
            if (log_count < 255) {
                strncpy(log[log_count++].text, ">>[Set Complete]---------------------", 64);
                log[log_count - 1].glyph = false;
            }
            printf(">>[Set Complete]---------------------\n\n"); fflush(stdout);
            selected_index = 0;
        }
    }

    // Right click to reset selection
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        selected_index = 0;
        printf("[Selection Reset]\n"); fflush(stdout);
        if (log_count < 255) {
            strncpy(log[log_count++].text, "[Selection Reset]", 64);
            log[log_count - 1].glyph = false;
        }
    }

    // Log viewer panel
    int log_x = grid_origin_x + (atlas_columns + 1) * glyph_draw_width + glyph_draw_width;
    int log_y = grid_origin_y - 2 * glyph_draw_height;
    int log_width = 22 * glyph_draw_width;
    int line_height = glyph_draw_height / 2;
    int visible_log_height = (GetScreenHeight() - 6) / line_height * line_height;

    DrawRectangle(log_x, log_y, log_width, visible_log_height, BLACK);
    DrawRectangleLines(log_x, log_y, log_width, visible_log_height, GREEN);

    int visible_entries = visible_log_height / line_height;
    int scroll_start = (log_count > visible_entries - 2) ? (log_count - (visible_entries - 2)) : 0;

    for (int i = scroll_start, visible_index = 0; i < log_count; i++, visible_index++) {
        Vector2 pos = { log_x + glyph_draw_width, log_y + (visible_index + 2) * line_height };
        DrawTextEx(fallback_font, log[i].text, pos, line_height, 0, GREEN);

        if (log[i].glyph) {
            Rectangle src = { log[i].x * cell_w, log[i].y * cell_h, cell_w, cell_h };
            Rectangle dst = { log_x + 4, pos.y, glyph_draw_width / 2, line_height };
            DrawTexturePro(glyph_atlas, src, dst, (Vector2){0}, 0, WHITE);
        }
    }

    // Preview panel
    int preview_start = (selected_index > 0) ? (log_count - selected_index) : preview_start_index;
    if (preview_start >= 0) {
        int preview_x = grid_origin_x - 28;
        int preview_y = grid_origin_y + (atlas_rows + 2) * glyph_draw_height - 28;
        int preview_width = 3 * glyph_draw_width;
        int preview_height = 4 * glyph_draw_height;

        DrawRectangle(preview_x - 4, preview_y - 4, preview_width + 8, preview_height + 8, BLACK);
        DrawRectangleLines(preview_x - 4, preview_y - 4, preview_width + 8, preview_height + 8, GREEN);

        int grid_index[3][3] = {
            { 0, 4, 1 },
            { 6, 8, 7 },
            { 2, 5, 3 }
        };

        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                int part_index = grid_index[y][x];
                int log_index = preview_start + part_index;
                if (log_index < 0 || log_index >= log_count || !log[log_index].glyph) continue;
                Rectangle src = { log[log_index].x * cell_w, log[log_index].y * cell_h, cell_w, cell_h };
                Rectangle dst = { preview_x + x * glyph_draw_width, preview_y + y * glyph_draw_height,
                                  glyph_draw_width, glyph_draw_height };
                DrawTexturePro(glyph_atlas, src, dst, (Vector2){0}, 0, WHITE);
            }
        }

        // Strip row: cap_l, strip, cap_r
        int strip_parts[3] = { 9, 11, 10 };
        int strip_y = preview_y + 3 * glyph_draw_height + 4;

        for (int i = 0; i < 3; i++) {
            int part = strip_parts[i];
            int log_index = preview_start + part;
            if (log_index < 0 || log_index >= log_count || !log[log_index].glyph) continue;
            Rectangle src = { log[log_index].x * cell_w, log[log_index].y * cell_h, cell_w, cell_h };
            Rectangle dst = { preview_x + i * glyph_draw_width, strip_y, glyph_draw_width, glyph_draw_height };
            DrawTexturePro(glyph_atlas, src, dst, (Vector2){0}, 0, WHITE);
        }
    }
}



//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
// --- TEST / MAIN -----------------------------------------------------------------------------------------------


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
  


    while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_TAB)) {show_tilepicker = !show_tilepicker;}

    tm_canvas_begin(&canvas);

    ClearBackground(BLACK); // A darker background to highlight UI elements


    tm_set_spacing(1);
    ALIGN(LEFT, TOP);
// --- Inside your main game loop, within tm_canvas_begin(&canvas) and tm_canvas_end(&canvas) ---

// Assuming tm_set_spacing(1); and ALIGN(LEFT, TOP); are set at the start of your main loop iteration

// --- Panel Frame A and B (Visual boundaries for Vbox A & B) ---
tm_panel(RECT(0,0,14,45)); // list frame A
tm_panel(RECT(12,0,13,45)); // list frame B

tm_panel_titled("TITLE",RECT(25,32,56,16),2); // LOG frame
ALIGN(LEFT,CENTER);
tm_vbox(RECT(26,33,54,16));
tm_label("", AUTO);
tm_label_panel("ACTIONS",SIZE(9,3),-1);
tm_text(">you ate the poopo bug", AUTO);
tm_text(">you ate the poopo bug", AUTO);
tm_text(">you ate the poopo bug", AUTO);
tm_text(">you ate the poopo bug", AUTO);


// --- VBOX A: List of Labels and Text (your original example) ---
tm_vbox(RECT(1,1,11,45)); // This sets gui_context for elements in this Vbox
    
    ALIGN(CENTER,CENTER);
    tm_text("ACTIONS",SIZE(11,1));

    ALIGN(CENTER,CENTER);
    tm_label_panel("ABILITY",SIZE(-1,3),0);
    tm_label_panel("  USE  ",AUTO,0);
    tm_label_panel(" TALK  ",AUTO,0);
    tm_label_panel(" FLIRT ",AUTO,0);

    ALIGN(LEFT,CENTER);
    tm_text("ITEMS",SIZE(11,1));

    ALIGN(CENTER,CENTER);
    tm_label("BENT STRAW",SIZE(11,3));
    tm_label("DOG FUR",SIZE(11,3));
    tm_label("MANS HAT",SIZE(11,3));
    tm_label("STRAW",SIZE(11,3));

// --- VBOX B: Panels with children (your expanded weirdos list) ---
// This call *overwrites* gui_context, ending Vbox A's context and starting Vbox B's
tm_vbox(RECT(13 ,2,14,45)); // start Vbox B. Increased width to 14 for content.
    
    ALIGN(LEFT,TOP); // Alignment for elements *within* Vbox B

    // Character 1: Gorgon
    grect p0 = tm_panel(SIZE(14,5)); // Panel 0: Auto-positioned within Vbox B, full width of 14, height 5
    tm_label("Gorgon",RELRECT(p0,0,0,8,1)); // Name label, explicit 8x1 size at 0,0 offset
    tm_text("PWR: 6",OFFSET(p0,1,1)); // Power text, auto-sized, at 1,1 offset
    tm_text("SKI: 3",OFFSET(p0,1,2)); // Skill text, auto-sized, at 1,2 offset
    tm_text(">GobSmack",OFFSET(p0,1,3)); // Ability text, auto-sized, at 1,3 offset


    // Character 2: Velbort
    grect p1 = tm_panel(SIZE(14,5)); // Panel 1: Auto-positioned within Vbox B
    tm_label("Velbort",RELRECT(p1,0,0,8,1));
    tm_text("PWR: 3",OFFSET(p1,1,1));
    tm_text("SKI: 4",OFFSET(p1,1,2));
    tm_text(">Drain Life",OFFSET(p1,1,3));

    // Character 4: Zarthus
    grect p3 = tm_panel(SIZE(14,5)); // Panel 3: Auto-positioned within Vbox B
    tm_label("Zarthus",RELRECT(p3,0,0,8,1));
    tm_text("PWR: 5",OFFSET(p3,1,1));
    tm_text("SKI: 7",OFFSET(p3,1,2));
    tm_text(">Teleport",OFFSET(p3,1,3));

    tm_canvas_end(&canvas);

        BeginDrawing();

        ClearBackground(BLACK); // Clear the actual window with black

if (show_tilepicker) {
    
    glyph_tool();      // Then draw your tilepicker
}

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