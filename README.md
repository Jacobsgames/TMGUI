# TMGUI (ALPHA)

**TMGUI** is a minimalistic 'TextMode' IMGUI (Immediate Mode GUI) API being developed in C with Raylib at its core (https://www.raylib.com/). Designed for pixel-perfect grid-based interfaces, tile-based games/toys/tools, and text-mode aesthetics. Inspired by old-school OS interfaces and modern TextMode aesthetics, TMGUI emphasizes predictability, simplicity and total layout control with a DSL like layout code syntax.

## Features
**Tile/Grid-Based Layout**  
- Fixed cell dimensions (`cell_w`, `cell_h`)  
- Grid-rect based positioning with optional auto-placement (`AUTO`)  

**Flat Layout System**  
- Supports `tm_vbox(...)` and `tm_hbox(...)` with deterministic cursor-based layout  
- No nesting — one container at a time  

**Basic Primitives**  
- `tm_label(...)`, `tm_button(...)`, `tm_rect(...)`, `tm_text(...)`, `tm_drawtile(...)`  

**Alignment Support**  
- Horizontal: `ALIGN_LEFT`, `ALIGN_CENTER`, `ALIGN_RIGHT`  
- Vertical: `ALIGN_TOP`, `ALIGN_CENTER`, `ALIGN_BOTTOM`  
- Controlled via `tm_align(ALIGN(H, V))` macro  

**Style System**  
- `tm_style` struct includes base styling + button states (`normal`, `hover`, `active`)  
- Per-widget style support using global `current_style`  
- Easily define reusable themes  

**AUTO Layout Support (Minimalist)**  
- Use `AUTO` in any gridrect to auto-size/position elements  
- Width expands in `tm_vbox`, height expands in `tm_hbox`  
- Supports macros like `SIZE(w,h)`, `WIDTH(w)`, `HEIGHT(h)`, `AUTO`  

**Simple GUI layout syntax**  
- Clean syntax for GUI layout:  
  
```c
tm_canvas canvas = tm_canvas_init(80, 45, false);
tm_canvas_begin(&canvas);

	tm_vbox(RECT(2, 2, 20, 0));
		tm_button("Play Game", AUTO);
		tm_set_spacing(2);
		tm_button("Options", AUTO);
		tm_button("Exit", AUTO);

		tm_align(ALIGN(CENTER, TOP));
		tm_label("Centered Title", SIZE(30, 1));

tm_canvas_end(&canvas);
```
 **No Dynamic Allocation**

No `malloc`s — all layout state is static and explicit.

**Zero Dependencies Beyond Raylib**

- Pure **C99**
- **Raylib** handles rendering, input, and font loading

🛠️**Planned Features**

-  Scroll marquee for clipped label text  
-  Input widgets (text field, slider, checkbox, spinbox, int/float input, dropdowns)
-  Scrollable areas (vertical and horizontal, with regular and log style scroll modes)
-  Tile based panels/buttons/labels
-  Animated tile support
-  Tooltip support
-  Expandable/collapsable panels
-  And much more!

---

📄 **License**

**MIT**

## Author

**Jacob Holland**  
Made with love.

README written by GPT as placeholder. 

