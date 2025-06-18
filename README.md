
![TMGUI demo](LOGO.png)

**TMGUI** is a minimalist 'TextMode' IMGUI Library (Immediate Mode GUI) being developed in C with Raylib at its core (https://www.raylib.com/). Designed for pixel-perfect grid-based interfaces for games, toys, tools, and text-mode aesthetics. Inspired by old-school OS interfaces, terminal displays and the modern TextMode art movement.<br> 
**TMGUI** emphasizes predictability, simplicity and total layout control with a DSL like syntax for UI layout code.

![TMGUI demo](EXAMPLE.gif)

## Features
**Grid-Based Layout and Container Support**  
- Fixed cell dimensions (`cell_w`, `cell_h`) defined with `tm_init(w,h)` 
- Supports `tm_vbox(...)` and `tm_hbox(...)` with deterministic cursor-based layout 
- Grid-rect based size and positioning with optional transform macros for use inside v/hboxes  
- Simple, flat layout system, no nested containers — one container at a time! 
- Supports `AUTO` (full auto), `POS(x,y)` (manual pos, auto size), `SIZE(w,h)` (auto pos, manual size) and `RECT(x,y,w,h)` for full manual control
- In `AUTO` or `POS(x,y)` mode, an elements width expands in `tm_vbox`, and height expands in `tm_hbox`  

**Grid-Based Pixel-Perfect Canvas Autoscaling**
- Define your tile dimentions with `tm_init(w,h)` 
- Define the canvas grid dimensions with `tm_canvas_init(w,h,trans)` (80x45 = 16:9)
- The auto scaling sizes your canvas to fit the current window size, adding letter/pillarboxing where needed
- Canvas will only scale to pixel perfect multiples, no weird artefacts! 

**GUI Elements and Primitives (many more to come!)**  
- `tm_label(...)`, `tm_button(...)`, `tm_rect(...)`, `tm_text(...)`, `tm_drawtile(...)`  

**Text Alignment and Element Spacing Support**
- Align text in Labels and Buttons!  
- Controlled via `ALIGN(RIGHT,CENTER)`, `ALIGNH(RIGHT)`, `ALIGNV(TOP)` macros  
- Space out elements in a vbox/hbox!
- Controlled via `tm_add_spacing(1)` to overide the current spacing  

**Style System**  
- `tm_style` struct controls an elements:
  - Foreground / background color
  - Border color / width
  - Font
  - Button states: normal, hover, active
- Per-element style with `tm_set_style(...)`
- Per-element font override with `tm_set_font(...)` (will overide the styles font)
- Easy to define multiple reusable styles (e.g., `STYLE_TMGUI`, `STYLE_GREY`)

**Clean DSL-like layout syntax**  
- Efficient syntax for GUI layout code:  
  
```c
tm_canvas canvas = tm_canvas_init(80, 45, false);
tm_canvas_begin(&canvas);

	tm_set_spacing(1);
	tm_label("MANUALY TRANSFORMED", RECT(20, 0, 20, 3));
	tm_label("MANUALY POSITIONED", POS(20, 4));

	tm_vbox(RECT(2, 2, 20, 0));

		tm_label("AUTO POS TITLE", SIZE(20, 3));
		tm_button("Play Game", AUTO);
		tm_button("Options", AUTO);
		tm_button("Exit", AUTO);

		tm_set_font(&YOURFONT);
		tm_set_style(&STYLE_YOURS);
		ALIGN(CENTER,CENTER);
		tm_label("CENTERED TITLE", SIZE(30, 3));

tm_canvas_end(&canvas);
```
**No Dynamic Allocation**
- No `malloc` or hidden heap usage
- Layout state is **entirely static and explicit**

**Zero Dependencies Beyond Raylib**
- Pure, low level **C99** codebase
- **Raylib** handles rendering, input, and font loading

🛠️**Planned Features**🛠️

-  Scroll marquee for clipped label text  
-  Input widgets (text field, slider, checkbox, spinbox, int/float input, dropdowns)
-  Scrollable areas (vertical and horizontal, with regular and log style scroll modes)
-  Tile based panels/buttons/labels
-  Animated tile support
-  Tooltip support
-  Expandable/collapsable panels
-  And much more!

---

📄 **License - MIT**

****

**Jacob Holland**  
Made with love.


