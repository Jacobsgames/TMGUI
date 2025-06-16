// tmgui.c
#include "tmgui.h"
#include "raylib.h"
#include <string.h>

// ——— Internal State ———
#define MAX_LAYOUT_STACK 16
static LayoutContext layout_stack[MAX_LAYOUT_STACK];
static int          layout_stack_top = -1;

// Constants
#define TILE_SIZE   8
#define GRID_WIDTH  80
#define GRID_HEIGHT 45
#define FRAME_WIDTH  (GRID_WIDTH * TILE_SIZE)
#define FRAME_HEIGHT (GRID_HEIGHT * TILE_SIZE)

static tm_style     current_style;
static Font         current_font;
static Texture2D    tile_atlas;
static bool         inited = false;

// ——— Helpers ———
static LayoutContext *current_ctx(void) {
    if (layout_stack_top < 0) return NULL;
    return &layout_stack[layout_stack_top];
}

// ——— Layout API ———
gridrect tm_next_cell(int w, int h) {
    LayoutContext *ctx = current_ctx();
    if (!ctx) {
        // no container: place at 0,0
        return (gridrect){ 0, 0, w, h };
    }
    // compute at origin + cursor
    int x = ctx->origin.x + ctx->cursor_x;
    int y = ctx->origin.y + ctx->cursor_y;
    // advance cursor based on mode
    if (ctx->mode == LAYOUT_HBOX) {
        ctx->cursor_x += w;
    } else if (ctx->mode == LAYOUT_VBOX) {
        ctx->cursor_y += h;
    }
    return (gridrect){ x, y, w, h };
}

void tm_vbox(gridrect r) {
    // determine anchor
    int ax = (r.x < 0 ? tm_next_cell(0,0).x : r.x);
    int ay = (r.y < 0 ? tm_next_cell(0,0).y : r.y);
    if (layout_stack_top < MAX_LAYOUT_STACK-1) {
        layout_stack[++layout_stack_top] = (LayoutContext){
            .mode     = LAYOUT_VBOX,
            .origin   = { ax, ay, 0, 0 },
            .cursor_x = 0,
            .cursor_y = 0,
        };
    }
}

void tm_hbox(gridrect r) {
    int ax = (r.x < 0 ? tm_next_cell(0,0).x : r.x);
    int ay = (r.y < 0 ? tm_next_cell(0,0).y : r.y);
    if (layout_stack_top < MAX_LAYOUT_STACK-1) {
        layout_stack[++layout_stack_top] = (LayoutContext){
            .mode     = LAYOUT_HBOX,
            .origin   = { ax, ay, 0, 0 },
            .cursor_x = 0,
            .cursor_y = 0,
        };
    }
}

void tm_end_box(void) {
    if (layout_stack_top >= 0) layout_stack_top--;
}

// ——— Core API ———
void tmgui_init(void) {
    if (inited) return;
    // load a default font (adjust path as needed)
    current_font = LoadFontEx("C:/Code/tmgui/fonts/FROGBLOCK.ttf", 8, NULL, 0);
    SetTextureFilter(current_font.texture, TEXTURE_FILTER_POINT);
    // load atlas (optional—only if you call tm_drawtile)
    tile_atlas = LoadTexture("C:/Code/tmgui/tiles/T_FROGBLOCK.png");
    SetTextureFilter(tile_atlas, TEXTURE_FILTER_POINT);
    // default style
    current_style = STYLE_TMGUI;
    inited = true;
}

void tmgui_shutdown(void) {
    if (!inited) return;
    UnloadFont(current_font);
    UnloadTexture(tile_atlas);
    inited = false;
}

void tm_set_style(const tm_style *style) {
    if (style) current_style = *style;
}

void tm_set_font(Font font) {
    current_font = font;
}

// ——— Drawing Primitives ———
void tm_rect(gridrect r) {
    int px = r.x * 8, py = r.y * 8;
    int pw = r.w * 8, ph = r.h * 8;
    DrawRectangle(px, py, pw, ph, current_style.background);
    if (current_style.border_width > 0) {
        DrawRectangleLinesEx(
            (Rectangle){ px, py, pw, ph },
            current_style.border_width,
            current_style.border
        );
    }
}

void tm_label(const char *text, int x, int y, Color c) {
    Vector2 pos = { x*8.f, y*8.f };
    DrawTextEx(current_font, text, pos, 8, 0, c);
}

bool tm_button(const char *label, gridrect r) {
    // compute size
    int txtw = MeasureTextEx(current_font, label, 8, 0).x / 8;
    int w = (r.w > 0 ? r.w : txtw + 2);
    int h = (r.h > 0 ? r.h : 1);
    // get rect
    gridrect use = (r.x<0 && r.y<0)
        ? tm_next_cell(w,h)
        : (gridrect){ r.x, r.y, w, h };
    // draw
    tm_rect(use);
    tm_label(label, use.x+1, use.y, current_style.foreground);
    // interaction (returns true on click)
    Rectangle pr = {
        use.x*8.f, use.y*8.f,
        use.w*8.f, use.h*8.f
    };
    Vector2 m = GetMousePosition();
    if (CheckCollisionPointRec(m, pr) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        return true;
    }
    return false;
}

void tm_drawtile(int x, int y, atlaspos tile) {
    Rectangle src = {
        tile.x*8.f, tile.y*8.f,
        8, 8
    };
    Rectangle dst = {
        x*8.f, y*8.f,
        8, 8
    };
    DrawTexturePro(tile_atlas, src, dst, (Vector2){0,0}, 0.f, WHITE);
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////         TEST MAIN  ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void) {
    // 1) Setup Raylib window & offscreen buffer
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(FRAME_WIDTH*2, FRAME_HEIGHT*2, "TMGUI Test");
    RenderTexture2D target = LoadRenderTexture(FRAME_WIDTH, FRAME_HEIGHT);
    SetTargetFPS(60);

    tmgui_init();

    // 2) Main loop
    while (!WindowShouldClose()) {
        // ——— Draw into our 80×45 tile canvas ———
        BeginTextureMode(target);
        ClearBackground(BLACK);

        // Your test UI here:
        // e.g. a little box and two buttons in a vbox
        tm_set_style(&STYLE_TMGUI);


        tm_vbox(R(0, 0, 0, 0));     // AUTO‑flow vbox at (4,4)
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
        tm_end_box();


        tm_vbox(R(7, 0, 0, 0));     // AUTO‑flow vbox at (4,4)
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
          tm_button("Hello", AUTO);
          tm_button("World", AUTO);
        tm_end_box();

        tm_drawtile(1, 3, TILE_B);

        EndTextureMode();  // ← you must EndTextureMode!



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////    DRAW V FRAME    ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // ——— Now stretch that 80×45 canvas up to the actual window ———
        BeginDrawing();
        ClearBackground(DARKGRAY);

        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        int scaleX = screenW / FRAME_WIDTH;
        int scaleY = screenH / FRAME_HEIGHT;
        int scale  = (scaleX < scaleY ? scaleX : scaleY);
        if (scale < 1) scale = 1;

        int drawW = FRAME_WIDTH * scale;
        int drawH = FRAME_HEIGHT * scale;
        int offsetX = (screenW - drawW) / 2;
        int offsetY = (screenH - drawH) / 2;

        DrawTexturePro(
            target.texture,
            (Rectangle){ 0, 0, FRAME_WIDTH, -FRAME_HEIGHT },
            (Rectangle){ offsetX, offsetY, drawW, drawH },
            (Vector2){ 0, 0 },
            0.0f, WHITE
        );

        EndDrawing();  // ← and don’t forget EndDrawing()
    }

    // 3) Shutdown
    tmgui_shutdown();
    CloseWindow();
    return 0;
}