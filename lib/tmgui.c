#include "tmgui.h"
#include "raylib.h"
#include <string.h>

#define MAX_LAYOUT_STACK 16
static LayoutContext layout_stack[MAX_LAYOUT_STACK];
static int           layout_stack_top = -1;

#define TILE_SIZE   8
#define GRID_WIDTH  80
#define GRID_HEIGHT 45
#define FRAME_WIDTH  (GRID_WIDTH * TILE_SIZE)
#define FRAME_HEIGHT (GRID_HEIGHT * TILE_SIZE)

static tm_style  current_style;
static Font      current_font;
static Texture2D tile_atlas;
static bool      inited = false;

// track mouse transform
static int g_tm_scale, g_tm_offX, g_tm_offY;

void tm_update_transform(int scale, int offX, int offY) {
    g_tm_scale = scale;
    g_tm_offX  = offX;
    g_tm_offY  = offY;
}

Vector2 tm_mouse_grid(void) {
    Vector2 m = GetMousePosition();
    float px = (m.x - g_tm_offX)/(float)g_tm_scale;
    float py = (m.y - g_tm_offY)/(float)g_tm_scale;
    return (Vector2){ px/TILE_SIZE, py/TILE_SIZE };
}

static LayoutContext *current_ctx(void) {
    return layout_stack_top >= 0
         ? &layout_stack[layout_stack_top]
         : NULL;
}

gridrect tm_next_cell(int w, int h) {
    LayoutContext *ctx = current_ctx();
    if (!ctx) return (gridrect){0,0,w,h};
    int x = ctx->origin.x + ctx->cursor_x;
    int y = ctx->origin.y + ctx->cursor_y;
    if (ctx->mode == LAYOUT_HBOX) ctx->cursor_x += w;
    else                          ctx->cursor_y += h;
    return (gridrect){ x, y, w, h };
}

void tm_vbox(gridrect r) {
    // Pop previous box (flat model, only one active at a time)
    if (layout_stack_top >= 0) layout_stack_top--;

    // Then push the new one
    int ax = (r.x<0 ? tm_next_cell(0,0).x : r.x);
    int ay = (r.y<0 ? tm_next_cell(0,0).y : r.y);
    if (layout_stack_top < MAX_LAYOUT_STACK-1) {
        layout_stack[++layout_stack_top] = (LayoutContext){
            .mode     = LAYOUT_VBOX,
            .origin   = { ax, ay, 0, 0 },
            .cursor_x = 0,
            .cursor_y = 0
        };
    }
}

void tm_hbox(gridrect r) {
    // Pop previous box (flat model, only one active at a time)
    if (layout_stack_top >= 0) layout_stack_top--;

    // Then push the new one
    int ax = (r.x<0 ? tm_next_cell(0,0).x : r.x);
    int ay = (r.y<0 ? tm_next_cell(0,0).y : r.y);
    if (layout_stack_top < MAX_LAYOUT_STACK-1) {
        layout_stack[++layout_stack_top] = (LayoutContext){
            .mode     = LAYOUT_HBOX,
            .origin   = { ax, ay, 0, 0 },
            .cursor_x = 0,
            .cursor_y = 0
        };
    }
}



void tmgui_init(void) {
    if (inited) return;
    current_font = LoadFontEx("C:/Code/tmgui/fonts/FROGBLOCK.ttf", TILE_SIZE, NULL, 0);
    SetTextureFilter(current_font.texture, TEXTURE_FILTER_POINT);
    tile_atlas   = LoadTexture("C:/Code/tmgui/tiles/T_FROGBLOCK.png");
    SetTextureFilter(tile_atlas, TEXTURE_FILTER_POINT);
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

tm_canvas tm_canvas_init(int grid_w, int grid_h, bool transparent) {
    tm_canvas c = {0};
    c.grid_w = grid_w; c.grid_h = grid_h; c.transparent = transparent;
    c.target = LoadRenderTexture(grid_w*TILE_SIZE, grid_h*TILE_SIZE);
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
    int sx = sw/(c->grid_w*TILE_SIZE), sy = sh/(c->grid_h*TILE_SIZE);
    c->scale = sx<sy? sx: sy; if (c->scale<1) c->scale=1;
    int dw = c->grid_w*TILE_SIZE*c->scale;
    int dh = c->grid_h*TILE_SIZE*c->scale;
    c->offset_x = (sw-dw)/2; c->offset_y = (sh-dh)/2;
    tm_update_transform(c->scale, c->offset_x, c->offset_y);
    DrawTexturePro(c->target.texture,
                   (Rectangle){0,0,c->grid_w*TILE_SIZE,-c->grid_h*TILE_SIZE},
                   (Rectangle){c->offset_x,c->offset_y,dw,dh},
                   (Vector2){0,0},0,WHITE);
}

void tm_rect(gridrect r) {
    DrawRectangle(r.x*TILE_SIZE,r.y*TILE_SIZE,r.w*TILE_SIZE,r.h*TILE_SIZE,
                  current_style.background);
    if (current_style.border_width>0)
        DrawRectangleLinesEx(
            (Rectangle){r.x*TILE_SIZE,r.y*TILE_SIZE,r.w*TILE_SIZE,r.h*TILE_SIZE},
            current_style.border_width,
            current_style.border);
}

void tm_label(const char *text, int x, int y, Color c) {
    DrawTextEx(current_font,text,(Vector2){x*TILE_SIZE,y*TILE_SIZE},
               TILE_SIZE,0,c);
}

bool tm_button(const char *label, gridrect r) {
    // 1) measure
    int txtw = MeasureTextEx(current_font, label, TILE_SIZE, 0).x / TILE_SIZE;
    int w = (r.w > 0 ? r.w : txtw + 2);
    int h = (r.h > 0 ? r.h : 1);

    LayoutContext *ctx = current_ctx();
    gridrect use;

    bool is_auto_left  = (r.x == -1 && r.y < 0);
    bool is_auto_right = (r.x == -2 && r.y < 0);

    if ((is_auto_left || is_auto_right) && ctx && ctx->mode == LAYOUT_VBOX) {
        // 2a) Get next Y without advancing
        int y = ctx->origin.y + ctx->cursor_y;
        int x = ctx->origin.x;

        int cw = (ctx->requested.w > 0 ? ctx->requested.w : 0);

        if (is_auto_right && cw > 0) {
            x = ctx->origin.x + (cw - w);
        }

        use = (gridrect){ x, y, w, h };

        // 2b) Manually advance cursor after placing
        ctx->cursor_y += h;
    } else if (r.x < 0 && r.y < 0) {
        // fallback auto-mode
        use = tm_next_cell(w, h);
    } else {
        use = (gridrect){ r.x, r.y, w, h };
    }

    // 3) input
    Vector2 mg = tm_mouse_grid();
    Rectangle pr = { use.x, use.y, use.w, use.h };
    bool over = CheckCollisionPointRec(mg, pr);
    bool pressed = over && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool clicked = over && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    // 4) style
    const tm_style *sty = &STYLE_BTN_NORMAL;
    if (pressed) sty = &STYLE_BTN_ACTIVE;
    else if (over) sty = &STYLE_BTN_HOVER;

    // 5) draw
    DrawRectangle(use.x * TILE_SIZE, use.y * TILE_SIZE, use.w * TILE_SIZE, use.h * TILE_SIZE, sty->background);
    if (sty->border_width > 0) {
        DrawRectangleLinesEx(
            (Rectangle){ use.x * TILE_SIZE, use.y * TILE_SIZE, use.w * TILE_SIZE, use.h * TILE_SIZE },
            sty->border_width, sty->border
        );
    }

    DrawTextEx(current_font, label,
               (Vector2){ use.x * TILE_SIZE + TILE_SIZE, use.y * TILE_SIZE },
               TILE_SIZE, 0, sty->foreground);

    return clicked;
}

void tm_drawtile(int x, int y, atlaspos tile) {
    DrawTexturePro(tile_atlas,
                   (Rectangle){tile.x*TILE_SIZE,tile.y*TILE_SIZE,TILE_SIZE,TILE_SIZE},
                   (Rectangle){x*TILE_SIZE,y*TILE_SIZE,TILE_SIZE,TILE_SIZE},
                   (Vector2){0,0},0,WHITE);
}


// ——— Test Main ———
int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(FRAME_WIDTH*2,FRAME_HEIGHT*2,"TMGUI Layout Test");
    SetTargetFPS(60);

    tmgui_init();
    tm_canvas canvas = tm_canvas_init(80,45,false);

    while (!WindowShouldClose()) {
        tm_canvas_begin(&canvas);


            tm_set_style(&STYLE_TMGUI);

              // left
              tm_vbox(R(0,-1,24,0));

                tm_button("Left One", AUTO);
                tm_button("Left Two", AUTO);
                tm_button("Left Three", AUTO);


              // center
              tm_vbox(R(10,-1,24,0));

                tm_button("Center A", AUTO);
                tm_button("Center B", AUTO);
                tm_button("Center C", AUTO);


              // right
              tm_vbox(R(20,-1,24,0));
                tm_button("CentA", AUTO);
                tm_button("Cen", AUTO);
                tm_button("Cente", RIGHT);




              tm_hbox(R(30, 10, 50, 0));
tm_button("One", AUTO);
tm_button("Two", AUTO);
tm_button("Three", AUTO);





        tm_canvas_end(&canvas);

        BeginDrawing();
            ClearBackground(DARKGRAY);
            // stretch canvas->screen
            DrawTexturePro(
                canvas.target.texture,
                (Rectangle){0,0,FRAME_WIDTH,-FRAME_HEIGHT},
                (Rectangle){canvas.offset_x,canvas.offset_y,
                            FRAME_WIDTH*canvas.scale,
                            FRAME_HEIGHT*canvas.scale},
                (Vector2){0,0},0,WHITE);
        EndDrawing();
    }

    tmgui_shutdown();
    CloseWindow();
    return 0;
}