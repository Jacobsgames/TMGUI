#include "tmgui.h"
#include <string.h>
#include <math.h>

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

// Converts horizontal pixel distance to grid cells (X-axis)
static inline int pixels_to_grid_x(float pixels) {
    return (int)(pixels / (float)cell_w);
}

// Converts vertical pixel distance to grid cells (Y-axis)
static inline int pixels_to_grid_y(float pixels) {
    return (int)(pixels / (float)cell_h);
}

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


// --- Canvas ---
ui_canvas ui_canvas_init(int grid_w, int grid_h, bool transparent) {
    ui_canvas c = {0};
    c.grid_w = grid_w;
    c.grid_h = grid_h;
    c.transparent = transparent;
    c.target = LoadRenderTexture(grid_w * cell_w, grid_h * cell_h);
    SetTextureFilter(c.target.texture, TEXTURE_FILTER_POINT);
    return c;
}

void ui_canvas_begin(ui_canvas *c) {
    BeginTextureMode(c->target);
    ClearBackground(c->transparent ? BLANK : BLACK);
}

void ui_canvas_end(ui_canvas *c) {
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

// --- Primitives ---
void tm_rect(gridrect r) {
    Rectangle pixel_rect = { r.x * cell_w, r.y * cell_h, r.w * cell_w, r.h * cell_h };
    DrawRectangle(pixel_rect.x, pixel_rect.y, pixel_rect.width, pixel_rect.height, current_style.base.background);
    if (current_style.base.border_width > 0)
        DrawRectangleLinesEx(pixel_rect, current_style.base.border_width, current_style.base.border);
}


void tm_text(const char *text, gridpos pos, Color c) {
    Font use_font = get_active_font();
    DrawTextEx(use_font, text, (Vector2){ pos.x * cell_w, pos.y * cell_h }, cell_h, 0, c);
}

void tm_label(const char *label, gridrect r) {
    Font use_font = get_active_font();
    Vector2 txt_width_px = MeasureTextEx(use_font, label, cell_h, 0);
    int txt_width = pixels_to_grid_x(txt_width_px.x);
    int w = (r.w > 0 ? r.w : txt_width + 2);
    int h = (r.h > 0 ? r.h : 1);

    gridrect final = (r.x < 0 && r.y < 0) ? tm_next_cell(w, h) : (gridrect){ r.x, r.y, w, h };

    Rectangle pixel_rect = {
        final.x * cell_w,
        final.y * cell_h,
        final.w * cell_w,
        final.h * cell_h
    };

    DrawRectangle(pixel_rect.x, pixel_rect.y, pixel_rect.width, pixel_rect.height, current_style.base.background);
    if (current_style.base.border_width > 0)
        DrawRectangleLinesEx(pixel_rect, current_style.base.border_width, current_style.base.border);

    // --- HORIZONTAL ALIGN ---
    int text_x = final.x + 1;
    if (h_align == ALIGN_CENTER)
        text_x = final.x + (final.w - txt_width) / 2;
    else if (h_align == ALIGN_RIGHT)
        text_x = final.x + final.w - txt_width - 1;

    // --- VERTICAL ALIGN ---
    int text_y = final.y;
    if (v_align == ALIGN_CENTER)
        text_y = final.y + (final.h - 1) / 2;
    else if (v_align == ALIGN_BOTTOM)
        text_y = final.y + final.h - 1;

    tm_text(label, (gridpos){ text_x, text_y }, current_style.base.foreground);
}

void tm_drawtile(gridpos pos, atlaspos tile) {
    DrawTexturePro(tile_atlas,
        (Rectangle){ tile.x * cell_w, tile.y * cell_h, cell_w, cell_h },
        (Rectangle){ pos.x * cell_w, pos.y * cell_h, cell_w, cell_h },
        (Vector2){ 0, 0 }, 0, WHITE);
}

bool tm_button(const char *label, gridrect recti) {
    Font active_font = get_active_font();
    Vector2 txt_width_px = MeasureTextEx(active_font, label, cell_h, 0);
    int txt_width = pixels_to_grid_x(txt_width_px.x);

    int w = (recti.w > 0 ? recti.w : txt_width + 2);
    int h = (recti.h > 0 ? recti.h : 1);

    gridrect final_rect = (recti.x < 0 && recti.y < 0) 
        ? tm_next_cell(w, h) 
        : (gridrect){ recti.x, recti.y, w, h };

    Vector2 mouse_grid = tm_mouse_grid();
    bool over = (mouse_grid.x >= final_rect.x && mouse_grid.x < final_rect.x + final_rect.w &&
                 mouse_grid.y >= final_rect.y && mouse_grid.y < final_rect.y + final_rect.h);

    bool pressed = over && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool clicked = over && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    tm_rect_style *button_style = &current_style.button.normal;
    if (pressed) button_style = &current_style.button.active;
    else if (over) button_style = &current_style.button.hover;

    Rectangle pixel_rect = {
        final_rect.x * cell_w,
        final_rect.y * cell_h,
        final_rect.w * cell_w,
        final_rect.h * cell_h
    };

    DrawRectangle(pixel_rect.x, pixel_rect.y, pixel_rect.width, pixel_rect.height, button_style->background);
    if (button_style->border_width > 0)
        DrawRectangleLinesEx(pixel_rect, button_style->border_width, button_style->border);

    // --- ALIGNMENT LOGIC ---
    int text_x = final_rect.x + 1;
    if (h_align == ALIGN_CENTER)
        text_x = final_rect.x + (final_rect.w - txt_width) / 2;
    else if (h_align == ALIGN_RIGHT)
        text_x = final_rect.x + final_rect.w - txt_width - 1;

    int text_y = final_rect.y;
    if (v_align == ALIGN_CENTER)
        text_y = final_rect.y + (final_rect.h - 1) / 2;
    else if (v_align == ALIGN_BOTTOM)
        text_y = final_rect.y + final_rect.h - 1;

    tm_text(label, (gridpos){ text_x, text_y }, button_style->foreground);

    return clicked;
}

// --- Layout ---
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
}

void tm_hbox(gridrect r) {
    gui_context.mode = LAYOUT_HBOX;
    gui_context.cursor_x = (r.x < 0 ? 0 : r.x);
    gui_context.cursor_y = (r.y < 0 ? 0 : r.y);
}


Font mycustomfont;

// --- Main ---
int main(void) {


    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int cw = 8, ch = 8, gw = 80, gh = 45;
    InitWindow(gw * cw * 2, gh * ch * 2, "TMGUI Style Test");
    tmgui_init(cw, ch);
    SetTargetFPS(60);

    mycustomfont = LoadFontEx("C:/Code/tmgui/fonts/CHUNKY.ttf", cell_h, NULL, 0);
    SetTextureFilter(mycustomfont.texture, TEXTURE_FILTER_POINT);

    ui_canvas canvas = ui_canvas_init(gw, gh, false);

    while (!WindowShouldClose()) {

         



        ui_canvas_begin(&canvas);
        
            tm_set_style(&STYLE_TMGUI);

            tm_set_spacing(0);
            DRAWTILE(1,1,TILE(10,10));
            tm_vbox(RECT(2, 2, 20, 0));

                ALIGN(CENTER,CENTER);
                tm_label("Centered label",SIZE(20, 3));

                ALIGNH(LEFT);
                tm_button("OPTION",SIZE(20, 1));
                tm_button("option",SIZE(20, 1));
                tm_button("settings ", SIZE(20, 1));
                tm_button("dogs",SIZE(20, 1));


                ALIGN(CENTER,CENTER);
                if (tm_button("PRINTS TO CONSOLE",SIZE(20, 5))) {
                    TraceLog(LOG_INFO, "Play button clicked!");
                }
                TEXT("This is drawn using the theme's font! set_font(NULL)", 1, 42, WHITE);

                tm_set_style(&STYLE_GREY);

                       ALIGNH(CENTER);
                tm_label("Centered label",SIZE(20, 1));

                ALIGN(CENTER,CENTER);
                tm_button("OPTION",SIZE(20, 3));
                tm_button("option",SIZE(20, 3));
                tm_button("settings ", SIZE(20, 3));
                tm_button("dogs",SIZE(20, 3));
                tm_set_spacing(4);
                tm_button("(start 4 space)",SIZE(20, 3));



        ui_canvas_end(&canvas);

        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }

    tmgui_shutdown();
    CloseWindow();
    return 0;
}
