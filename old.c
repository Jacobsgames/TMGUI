

//tmgui.h
#ifndef TMGUI_H
#define TMGUI_H

#include "raylib.h"




// Use tile coordinates instead of flat indices
typedef struct {
    int x; // column
    int y; // row
} atlaspos;
#define TILE(x, y) ((atlaspos){ (x), (y) })

#define TILE_A (TILE(13, 13)) // i.e. tile at column 2, row 4
#define TILE_B (TILE(14, 15))
#define TILE_C  (TILE(15, 15))


typedef struct {
    int x, y;
    int w, h;
} gridrect;
#define R(x, y, w, h) ((gridrect){ (x), (y), (w), (h) })
#define AUTO (gridrect){ -1, -1, 0, 1 }  // x/y from layout, width auto, height = 1

typedef struct {
    Color background;
    Color foreground;
    Color border;
    int border_width;
} tm_style;
#define TMGUI_STYLE (tm_style){ .background = DARKGRAY, .foreground = LIGHTGRAY, .border = GRAY, .border_width = 1 }



// API
void tmgui_init(void);
void tmgui_shutdown(void);

//Flow mods
void tm_set_font(Font font);
void tm_set_style(tm_style style);

//Elements
void tm_label(const char *text, int x, int y, Color color);
void tm_rect(gridrect r);
void tm_button(const char *label, int x, int y);
void tm_drawtile(int x, int y, atlaspos tile);

void tm_vbox(int x, int y);
void tm_hbox(int x, int y);
void tm_end_box(void);

#endif 

//tmgui.c

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

// For nested layouts
#define MAX_LAYOUT_STACK 16
static gridrect layout_stack[MAX_LAYOUT_STACK];
static int layout_stack_top = -1;
// internally track mode: 0 = none, 1 = vbox, 2 = hbox
static enum { LAYOUT_NONE, LAYOUT_VBOX, LAYOUT_HBOX } current_layout = LAYOUT_NONE;

// Tile atlas texture loaded once
static Texture2D tile_atlas = { 0 };

// Tiles per row in the atlas (assumes atlas is grid of tiles, square tiles of TILE_SIZE)
#define TILES_PER_ROW 16

static bool tmgui_initialized = false;
Font TMGUI_DEFAULT_FONT = { 0 };  // Start as invalid
static tm_style ACTIVE_STYLE = TMGUI_STYLE;

void tm_push_layout(gridrect cursor) {
    if (layout_stack_top < MAX_LAYOUT_STACK - 1) {
        layout_stack[++layout_stack_top] = layout_cursor;
        layout_cursor = cursor;
    }
}

void tm_pop_layout(void) {
    if (layout_stack_top >= 0) {
        layout_cursor = layout_stack[layout_stack_top--];
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

void tm_button(const char *label, int x, int y) {
    if (current_layout != LAYOUT_NONE) {
        x = layout_cursor.x;
        y = layout_cursor.y;
    }

    int text_len = MeasureTextEx(TMGUI_DEFAULT_FONT, label, TILE_SIZE, 0).x / TILE_SIZE;
    int width = text_len + 2;

    gridrect r = { x, y, width, 1 };
    tm_rect(r);
    tm_label(label, x + 1, y, ACTIVE_STYLE.foreground);

    // Advance layout
    if (current_layout == LAYOUT_VBOX) {
        layout_cursor.y += 1;
    } else if (current_layout == LAYOUT_HBOX) {
        layout_cursor.x += width;
    }
}

/*
void tm_button(const char *label, int x, int y) {
    int text_len = MeasureTextEx(TMGUI_DEFAULT_FONT, label, TILE_SIZE, 0).x / TILE_SIZE;
    int width = text_len + 2; // padding
    int height = 1;

    gridrect r = { x, y, width, height };
    tm_rect(r); // Uses style

    // Draw label
    tm_label(label, x + 1, y, ACTIVE_STYLE.foreground);
}*/

void tm_drawtile(int x, int y, atlaspos tile) {
    int tile_index = tile.y * TILES_PER_ROW + tile.x;
    int tx = tile.x * TILE_SIZE;
    int ty = tile.y * TILE_SIZE;

    Rectangle source = { (float)tx, (float)ty, (float)TILE_SIZE, (float)TILE_SIZE };
    Rectangle dest = { (float)(x * TILE_SIZE), (float)(y * TILE_SIZE), (float)TILE_SIZE, (float)TILE_SIZE };

    DrawTexturePro(tile_atlas, source, dest, (Vector2){0,0}, 0.0f, WHITE);
}

void tm_vbox(int x, int y) {
    current_layout = LAYOUT_VBOX;
    tm_push_layout((gridrect){ x, y, 0, 0 });
}

void tm_hbox(int x, int y) {
    current_layout = LAYOUT_HBOX;
    tm_push_layout((gridrect){ x, y, 0, 0 });
}

void tm_end_box(void) {
    tm_pop_layout();
    current_layout = LAYOUT_NONE;
}






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



        tm_label("ABCdef", 3, 10, WHITE);
        tm_label("Dungeons of dogfood", 3, 11, WHITE);
        tm_label("1234567890", 3, 12, WHITE);
        tm_label("ABCdef", 3, 10, WHITE);
        tm_label("Sphinx of black quartz, judge my vow.", 3, 13, WHITE);

        tm_set_style((tm_style){ RED, YELLOW, RED, 2 });
        tm_button("Danger!", 3, 3);

        tm_vbox(1,25);

        tm_set_style(TMGUI_STYLE);
        tm_button("Play", 3, 5);
        tm_button("BIG BOOTYS", 3, 6);
        tm_button("FREE DOGS", 3, 7);
        tm_button("ALL DAY EVERU WAY", 3, 8);
        tm_button("$56 entry", 3, 9);
        tm_button("FREE DOGS", 3, 10);
        tm_end_box();

  
        tm_drawtile(1, 3, TILE_B);

        for (int x = 0; x < GRID_WIDTH; x++){
            if (x % 2 == 0){
                tm_drawtile(x, 1, TILE_C);
            }else{
                tm_drawtile(x, 1, TILE_B);
            }

        }
                for (int x = 0; x < GRID_WIDTH; x++){
            if (x % 2 != 0){
                tm_drawtile(x, 0, TILE_C);
            }else{
                tm_drawtile(x, 0, TILE_B);
            }

        }


                
    
        













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





