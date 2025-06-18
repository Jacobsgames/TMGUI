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

static tm_align_mode g_tm_align = ALIGNMENT_LEFT;

// --- Transform ---
void tm_update_transform(int scale, int pos_x, int pos_y) {
    canvas_scale = scale;
    canvas_x = pos_x;
    canvas_y = pos_y;
}

Vector2 tm_mouse_grid(void) {
    Vector2 m = GetMousePosition();
    float px = (m.x - canvas_x) / (float)canvas_scale;
    float py = (m.y - canvas_y) / (float)canvas_scale;
    return (Vector2){ px / cell_w, py / cell_h };
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

void tm_align(tm_align_mode mode) {
    g_tm_align = mode;
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

// --- Primitives ---
void tm_rect(gridrect r) {
    Rectangle px = { r.x * cell_w, r.y * cell_h, r.w * cell_w, r.h * cell_h };
    DrawRectangle(px.x, px.y, px.width, px.height, current_style.base.background);
    if (current_style.base.border_width > 0)
        DrawRectangleLinesEx(px, current_style.base.border_width, current_style.base.border);
}

static Font get_active_font(void) {
    if (current_font.texture.id != 0) return current_font;
    if (current_style.font.texture.id != 0) return current_style.font;
    return fallback_font;
}

void tm_text(const char *text, int x, int y, Color c) {
    Font use_font = get_active_font();
    DrawTextEx(use_font, text, (Vector2){ x * cell_w, y * cell_h }, cell_h, 0, c);
}

void tm_label(const char *label, gridrect r) {
    Font use_font = get_active_font();
    Vector2 sz = MeasureTextEx(use_font, label, cell_h, 0);
    int tw = sz.x / cell_w;
    int w = (r.w > 0 ? r.w : tw + 2);
    int h = (r.h > 0 ? r.h : 1);

    gridrect final = (r.x < 0 && r.y < 0) ? tm_next_cell(w, h) : (gridrect){ r.x, r.y, w, h };

    Rectangle px = { final.x * cell_w, final.y * cell_h, final.w * cell_w, final.h * cell_h };
    DrawRectangle(px.x, px.y, px.width, px.height, current_style.base.background);
    if (current_style.base.border_width > 0)
        DrawRectangleLinesEx(px, current_style.base.border_width, current_style.base.border);

    // lignment logic here
    int text_x = final.x + 1; // Default: left
    if (g_tm_align == ALIGNMENT_CENTER)
        text_x = final.x + (final.w - tw) / 2;
    else if (g_tm_align == ALIGNMENT_RIGHT)
        text_x = final.x + final.w - tw - 1;

    tm_text(label, text_x, final.y, current_style.base.foreground);
}

void tm_drawtile(int x, int y, atlaspos tile) {
    DrawTexturePro(tile_atlas,
        (Rectangle){ tile.x * cell_w, tile.y * cell_h, cell_w, cell_h },
        (Rectangle){ x * cell_w, y * cell_h, cell_w, cell_h },
        (Vector2){ 0, 0 }, 0, WHITE);
}

bool tm_button(const char *label, gridrect recti) {
    Font use_font = get_active_font();
    Vector2 sz = MeasureTextEx(use_font, label, cell_h, 0);
    int txtw = (int)ceilf(sz.x / (float)cell_w);  // critical: prevent truncation issues
    int w = (recti.w > 0 ? recti.w : txtw + 2);
    int h = (recti.h > 0 ? recti.h : 1);

    gridrect use = (recti.x < 0 && recti.y < 0) ? tm_next_cell(w, h) : (gridrect){ recti.x, recti.y, w, h };
    Vector2 mg = tm_mouse_grid();
    Rectangle grid_rect = { use.x, use.y, use.w, use.h };

    bool over = CheckCollisionPointRec(mg, grid_rect);
    bool pressed = over && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool clicked = over && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    tm_rect_style *style = &current_style.button.normal;
    if (pressed)      style = &current_style.button.active;
    else if (over)    style = &current_style.button.hover;

    Rectangle px = { use.x * cell_w, use.y * cell_h, use.w * cell_w, use.h * cell_h };
    DrawRectangle(px.x, px.y, px.width, px.height, style->background);
    if (style->border_width > 0)
        DrawRectangleLinesEx(px, style->border_width, style->border);

    // --- Alignment ---
    int text_x = use.x + 1;
    if (g_tm_align == ALIGNMENT_CENTER)
        text_x = use.x + (use.w - txtw) / 2;
    else if (g_tm_align == ALIGNMENT_RIGHT)
        text_x = use.x + use.w - txtw - 1;

    tm_text(label, text_x, use.y, style->foreground);

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

    /*
    tm_style theme = {
        .base = STYLE_CONSOLE,
        .button = {
            .normal = STYLE_BTN_NORMAL,
            .hover  = STYLE_BTN_HOVER,
            .active = STYLE_BTN_ACTIVE,
        },
        .font = current_font
    };
    */

   

    tm_set_style(&STYLE_TMGUI);

    while (!WindowShouldClose()) {

         



        ui_canvas_begin(&canvas);

        tm_vbox(RECT(2, 2, 20, 0));
        if (tm_button("THIS PRINTS TO CONSOLE", AUTO)) {
            TraceLog(LOG_INFO, "Play button clicked!");
        }
        tm_set_spacing(1);
        tm_button("spaced", AUTO);
        tm_button("out", AUTO);
        tm_button("BUTTONS!", AUTO);
        ALIGN_CENTER;
        tm_button("CENTER + SIZED",SIZE(16, 5));
        tm_set_spacing(0);
        tm_label("Centered label",SIZE(30, 1));
        ALIGN_RIGHT;
        tm_label("Right aligned",SIZE(30, 1));
        ALIGN_LEFT;
        tm_label("LEFT aligned",SIZE(30, 1));
        ALIGN_CENTER;
        tm_button("Centered ", SIZE(30, 1));
        ALIGN_RIGHT;
        tm_button("Right aligned",SIZE(30, 1));
        ALIGN_LEFT;
        tm_button("LEFT aligned",SIZE(30, 1));
        
        tm_set_font(&mycustomfont);
        ALIGN_RIGHT;
        tm_label("Right custom font",SIZE(30, 1));
        tm_label("defined by user",SIZE(30, 1));
        tm_label("Right aligned",SIZE(30, 1));
        tm_label("now after this, load fallback font",SIZE(30, 1));
        

        tm_set_font(NULL);
        tm_text("This is drawn using the theme's font! set_font(NULL)", 1, 42, WHITE);

        ui_canvas_end(&canvas);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        EndDrawing();
    }

    tmgui_shutdown();
    CloseWindow();
    return 0;
}
