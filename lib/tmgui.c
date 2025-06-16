#include "tmgui.h"
#include "raylib.h"
#include <stddef.h>
#include <math.h>
#include <string.h>

// Constants
#define TILE_SIZE   8
#define GRID_WIDTH  80
#define GRID_HEIGHT 45
#define FRAME_WIDTH  (GRID_WIDTH * TILE_SIZE)
#define FRAME_HEIGHT (GRID_HEIGHT * TILE_SIZE)

gridrect layout_cursor = { 0, 0, 0, 0 };  // start position and size

static LayoutContext layout_stack[MAX_LAYOUT_STACK];
static int layout_stack_top = -1;

// Tile atlas texture loaded once
static Texture2D tile_atlas = { 0 };

// Tiles per row in the atlas (assumes atlas is grid of tiles, square tiles of TILE_SIZE)
#define TILES_PER_ROW 16

static bool tmgui_initialized = false;
Font TMGUI_DEFAULT_FONT = { 0 };  // Start as invalid
static tm_style ACTIVE_STYLE = STYLE_TMGUI;

static LayoutContext *current_ctx(void) {
    return (layout_stack_top >= 0) ? &layout_stack[layout_stack_top] : NULL;
}

void tm_push_layout(LayoutMode mode, int x, int y) {
    if (layout_stack_top < MAX_LAYOUT_STACK - 1) {
        LayoutContext ctx = {
            .mode = mode,
            .cursor = R(x, y, 0, 0),
            .origin = R(x, y, 0, 0),
        };
        layout_stack[++layout_stack_top] = ctx;
    }
}

void tm_pop_layout(void) {
    if (layout_stack_top >= 0) {
        layout_stack_top--;
    }
}

void tm_set_default_font(Font font) {
    TMGUI_DEFAULT_FONT = font;
}

void tm_set_style(tm_style style) {
    ACTIVE_STYLE = style;
}

void tmgui_init(void) {
    if (!tmgui_initialized) {
        TMGUI_DEFAULT_FONT = LoadFontEx("C:/Code/tmgui/fonts/FROGBLOCK.ttf", TILE_SIZE, NULL, 0);
        SetTextureFilter(TMGUI_DEFAULT_FONT.texture, TEXTURE_FILTER_POINT);
        tile_atlas = LoadTexture("C:/Code/tmgui/tiles/T_FROGBLOCK.png"); // Load your tile atlas here
        SetTextureFilter(tile_atlas, TEXTURE_FILTER_POINT);


        tmgui_initialized = true;
    }
}

void tmgui_shutdown(void) {
    if (tmgui_initialized) {
        UnloadFont(TMGUI_DEFAULT_FONT);
        tmgui_initialized = false;
    }
}


gridrect tm_next_cell(int w, int h) {
    LayoutContext *ctx = current_ctx();
    if (!ctx) {
        TraceLog(LOG_ERROR, "tm_next_cell called with no active layout context!");
        return R(0, 0, w, h);  // Fail safe
    }

    gridrect r = {
        .x = ctx->cursor.x,
        .y = ctx->cursor.y,
        .w = w,
        .h = h
    };

    if (ctx->mode == LAYOUT_VBOX)
        ctx->cursor.y += h;
    else if (ctx->mode == LAYOUT_HBOX)
        ctx->cursor.x += w;

    return r;
}



void tm_rect(gridrect r) {
    int px = r.x * TILE_SIZE;
    int py = r.y * TILE_SIZE;
    int pw = r.w * TILE_SIZE;
    int ph = r.h * TILE_SIZE;

    // Fill
    DrawRectangle(px, py, pw, ph, ACTIVE_STYLE.background);

    // Border
    if (ACTIVE_STYLE.border_width > 0) {
        DrawRectangleLinesEx(
            (Rectangle){px, py, pw, ph},
            ACTIVE_STYLE.border_width,
            ACTIVE_STYLE.border
        );
    }
}


void tm_label(const char *text, int x, int y, Color color) {
    Vector2 pos = { x * TILE_SIZE, y * TILE_SIZE };
    DrawTextEx(TMGUI_DEFAULT_FONT, text, pos, TILE_SIZE, 0, color);
}


void tm_button(const char *label, gridrect r) {
    LayoutContext *ctx = current_ctx();
    bool auto_layout = (r.x < 0 || r.y < 0);

    int text_width = MeasureTextEx(TMGUI_DEFAULT_FONT, label, TILE_SIZE, 0).x / TILE_SIZE;
    int w = (r.w > 0) ? r.w : text_width + 2;
    int h = (r.h > 0) ? r.h : 1;

    if (auto_layout) {
        if (!ctx) {
            TraceLog(LOG_ERROR, "tm_button: no layout context but AUTO used");
            r = R(0, 0, w, h);  // fallback
        } else {
            r = tm_next_cell(w, h);
        }
    }

    tm_rect(r);
    tm_label(label, r.x + 1, r.y, ACTIVE_STYLE.foreground);
}




void tm_drawtile(int x, int y, atlaspos tile) {
    int tile_index = tile.y * TILES_PER_ROW + tile.x;
    int tx = tile.x * TILE_SIZE;
    int ty = tile.y * TILE_SIZE;

    Rectangle source = { (float)tx, (float)ty, (float)TILE_SIZE, (float)TILE_SIZE };
    Rectangle dest = { (float)(x * TILE_SIZE), (float)(y * TILE_SIZE), (float)TILE_SIZE, (float)TILE_SIZE };

    DrawTexturePro(tile_atlas, source, dest, (Vector2){0,0}, 0.0f, WHITE);
}

void tm_hbox(gridrect r) {
    bool auto_layout = (r.x < 0 || r.y < 0);
    int w = (r.w > 0) ? r.w : 1;
    int h = (r.h > 0) ? r.h : 1;

    gridrect anchor = auto_layout ? tm_next_cell(w, h) : r;
    tm_push_layout(LAYOUT_HBOX, anchor.x, anchor.y);
}

void tm_vbox(gridrect r) {
    bool auto_layout = (r.x < 0 || r.y < 0);
    int w = (r.w > 0) ? r.w : 1;
    int h = (r.h > 0) ? r.h : 1;

    gridrect anchor = auto_layout ? tm_next_cell(w, h) : r;
    tm_push_layout(LAYOUT_VBOX, anchor.x, anchor.y);
}

void tm_end_box(void) {
    tm_pop_layout();
}





/////TESTING
int main() {
   
SetConfigFlags(FLAG_WINDOW_RESIZABLE);
SetTargetFPS(60);    
InitWindow(FRAME_WIDTH*2,FRAME_HEIGHT*2, "tmgui");
RenderTexture2D target = LoadRenderTexture(FRAME_WIDTH, FRAME_HEIGHT);
tmgui_init();



    while (!WindowShouldClose()) {
        BeginTextureMode(target); //START GUI DRAW
        ClearBackground(BLACK);
        

        
        //new
        tm_rect(R(20, 20, 15, 15));






        

tm_set_style(STYLE_TMGUI);
tm_hbox(R(0,0,30,10));  
    tm_vbox(R(-1,-1, 8,10));  

        tm_button("Left A", AUTO);
        tm_button("Left B", AUTO);
        tm_button("1234567", AUTO);
        tm_button("1234567", AUTO);
        tm_button("1234567", AUTO);
        tm_button("1234567", AUTO);
        tm_button("1234567", AUTO);

    tm_end_box();
    tm_set_style(STYLE_BW);

    tm_vbox(R(-1,-1, 8,10)); 

        tm_button("Right A", AUTO);
        tm_button("Right B", AUTO);
                tm_button("Right A", AUTO);
        tm_button("Right B", AUTO);
                tm_button("Right A", AUTO);
        tm_button("Right B", AUTO);
                tm_button("Right A", AUTO);
        tm_button("Right B", AUTO);
                tm_button("Right A", AUTO);
        tm_button("Right B", AUTO);
                tm_button("Right A", AUTO);
        tm_button("Right B", AUTO);


    tm_end_box();
tm_end_box();

  
        tm_drawtile(1, 3, TILE_B);




                
    
        













        EndTextureMode(); //END GUI DRAW

        BeginDrawing();//DRAW VIRTUAL FRAME FOR PIXEL SCALING
        ClearBackground(DARKGRAY);

        // Get real screen size
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        // Compute scale factor and position
        int scaleX = screenW / FRAME_WIDTH;
        int scaleY = screenH / FRAME_HEIGHT;
        int scale = (scaleX < scaleY) ? scaleX : scaleY;
        if (scale < 1) scale = 1;  // Prevent downscaling
        int scaledW = FRAME_WIDTH * scale;
        int scaledH = FRAME_HEIGHT * scale;
        int offsetX = (screenW - scaledW) / 2;
        int offsetY = (screenH - scaledH) / 2;
        // Draw virtual texture scaled into real screen
        DrawTexturePro(target.texture,(Rectangle){ 0, 0, FRAME_WIDTH, -FRAME_HEIGHT },(Rectangle){ offsetX, offsetY, scaledW, scaledH },(Vector2){ 0, 0 },0.0f,WHITE);
        EndDrawing();
    }
 tmgui_shutdown();
    CloseWindow();
    return 0;
}



