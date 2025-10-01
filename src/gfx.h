#ifndef GFX_H
#define GFX_H

//@table(name, display_string, cfg_string)
#define GFX_KeyXList(Key) \
	Key(Null, "Invalid Key", "null") \
	Key(Esc, "Escape", "esc") \
	Key(F1, "F1", "f1") \
	Key(F2, "F2", "f2") \
	Key(F3, "F3", "f3") \
	Key(F4, "F4", "f4") \
	Key(F5, "F5", "f5") \
	Key(F6, "F6", "f6") \
	Key(F7, "F7", "f7") \
	Key(F8, "F8", "f8") \
	Key(F9, "F9", "f9") \
	Key(F10, "F10", "f10") \
	Key(F11, "F11", "f11") \
	Key(F12, "F12", "f12") \
	Key(F13, "F13", "f13") \
	Key(F14, "F14", "f14") \
	Key(F15, "F15", "f15") \
	Key(F16, "F16", "f16") \
	Key(F17, "F17", "f17") \
	Key(F18, "F18", "f18") \
	Key(F19, "F19", "f19") \
	Key(F20, "F20", "f20") \
	Key(F21, "F21", "f21") \
	Key(F22, "F22", "f22") \
	Key(F23, "F23", "f23") \
	Key(F24, "F24", "f24") \
	Key(Tick, "Tick", "tick") \
	Key(0, "0", "0") \
	Key(1, "1", "1") \
	Key(2, "2", "2") \
	Key(3, "3", "3") \
	Key(4, "4", "4") \
	Key(5, "5", "5") \
	Key(6, "6", "6") \
	Key(7, "7", "7") \
	Key(8, "8", "8") \
	Key(9, "9", "9") \
	Key(Minus, "Minus", "minus") \
	Key(Equal, "Equal", "equal") \
	Key(Backspace, "Backspace", "backspace") \
	Key(Tab, "Tab", "tab") \
	Key(Q, "Q", "q") \
	Key(W, "W", "w") \
	Key(E, "E", "e") \
	Key(R, "R", "r") \
	Key(T, "T", "t") \
	Key(Y, "Y", "y") \
	Key(U, "U", "u") \
	Key(I, "I", "i") \
	Key(O, "O", "o") \
	Key(P, "P", "p") \
	Key(LeftBracket, "Left Bracket", "left_bracket") \
	Key(RightBracket, "Right Bracket", "right_bracket") \
	Key(BackSlash, "Back Slash", "backslash") \
	Key(CapsLock, "Caps Lock", "caps_lock") \
	Key(A, "A", "a") \
	Key(S, "S", "s") \
	Key(D, "D", "d") \
	Key(F, "F", "f") \
	Key(G, "G", "g") \
	Key(H, "H", "h") \
	Key(J, "J", "j") \
	Key(K, "K", "k") \
	Key(L, "L", "l") \
	Key(Semicolon, "Semicolon", "semicolon") \
	Key(Quote, "Quote", "quote") \
	Key(Return, "Return", "return") \
	Key(Shift, "Shift", "shift") \
	Key(Z, "Z", "z") \
	Key(X, "X", "x") \
	Key(C, "C", "c") \
	Key(V, "V", "v") \
	Key(B, "B", "b") \
	Key(N, "N", "n") \
	Key(M, "M", "m") \
	Key(Comma, "Comma", "comma") \
	Key(Period, "Period", "period") \
	Key(Slash, "Slash", "slash") \
	Key(Ctrl, "Ctrl", "ctrl") \
	Key(Alt, "Alt", "alt") \
	Key(Space, "Space", "space") \
	Key(Menu, "Menu", "menu") \
	Key(ScrollLock, "Scroll Lock", "scroll_lock") \
	Key(Pause, "Pause", "pause") \
	Key(Insert, "Insert", "insert") \
	Key(Home, "Home", "home") \
	Key(PageUp, "Page Up", "page_up") \
	Key(Delete, "Delete", "delete") \
	Key(End, "End", "end") \
	Key(PageDown, "Page Down", "page_down") \
	Key(Up, "Up", "up") \
	Key(Left, "Left", "left") \
	Key(Down, "Down", "down") \
	Key(Right, "Right", "right") \
	Key(Ex0, "Ex0", "ex0") \
	Key(Ex1, "Ex1", "ex1") \
	Key(Ex2, "Ex2", "ex2") \
	Key(Ex3, "Ex3", "ex3") \
	Key(Ex4, "Ex4", "ex4") \
	Key(Ex5, "Ex5", "ex5") \
	Key(Ex6, "Ex6", "ex6") \
	Key(Ex7, "Ex7", "ex7") \
	Key(Ex8, "Ex8", "ex8") \
	Key(Ex9, "Ex9", "ex9") \
	Key(Ex10, "Ex10", "ex10") \
	Key(Ex11, "Ex11", "ex11") \
	Key(Ex12, "Ex12", "ex12") \
	Key(Ex13, "Ex13", "ex13") \
	Key(Ex14, "Ex14", "ex14") \
	Key(Ex15, "Ex15", "ex15") \
	Key(Ex16, "Ex16", "ex16") \
	Key(Ex17, "Ex17", "ex17") \
	Key(Ex18, "Ex18", "ex18") \
	Key(Ex19, "Ex19", "ex19") \
	Key(Ex20, "Ex20", "ex20") \
	Key(Ex21, "Ex21", "ex21") \
	Key(Ex22, "Ex22", "ex22") \
	Key(Ex23, "Ex23", "ex23") \
	Key(Ex24, "Ex24", "ex24") \
	Key(Ex25, "Ex25", "ex25") \
	Key(Ex26, "Ex26", "ex26") \
	Key(Ex27, "Ex27", "ex27") \
	Key(Ex28, "Ex28", "ex28") \
	Key(Ex29, "Ex29", "ex29") \
	Key(NumLock, "Num Lock", "num_lock") \
	Key(NumSlash, "Numpad Slash", "numpad_slash") \
	Key(NumStar, "Numpad Star", "numpad_star") \
	Key(NumMinus, "Numpad Minus", "numpad_minus") \
	Key(NumPlus, "Numpad Plus", "numpad_plus") \
	Key(NumPeriod, "Numpad Period", "numpad_period") \
	Key(Num0, "Numpad 0", "numpad_0") \
	Key(Num1, "Numpad 1", "numpad_1") \
	Key(Num2, "Numpad 2", "numpad_2") \
	Key(Num3, "Numpad 3", "numpad_3") \
	Key(Num4, "Numpad 4", "numpad_4") \
	Key(Num5, "Numpad 5", "numpad_5") \
	Key(Num6, "Numpad 6", "numpad_6") \
	Key(Num7, "Numpad 7", "numpad_7") \
	Key(Num8, "Numpad 8", "numpad_8") \
	Key(Num9, "Numpad 9", "numpad_9") \
	Key(LeftMouseButton, "Left Mouse Button", "left_mouse") \
	Key(MiddleMouseButton, "Middle Mouse Button", "middle_mouse") \
	Key(RightMouseButton, "Right Mouse Button", "right_mouse") \

typedef enum GFX_Key {
#define GFX_KeyEnum(name, display_string, cfg_string) Glue(GFX_Key_, name),
	GFX_KeyXList(GFX_KeyEnum)
	GFX_Key_COUNT,
} GFX_Key;

typedef u32 GFX_Modifiers; enum {
	GFX_Modifier_Ctrl  = (1<<0),
	GFX_Modifier_Shift = (1<<1),
	GFX_Modifier_Alt   = (1<<2),
};

typedef struct GFX_Handle { u64 u64; } GFX_Handle;

typedef enum GFX_EventType {
	GFX_EventType_Null,
	GFX_EventType_Press,
	GFX_EventType_Release,
	GFX_EventType_Scroll,
	GFX_EventType_MouseMove,
	GFX_EventType_WindowClose,
	GFX_EventType_WindowLoseFocus,
	GFX_EventType_COUNT,
} GFX_EventType;

typedef struct GFX_Event {
	struct GFX_Event *next;
	struct GFX_Event *prev;
	GFX_Handle window;
	GFX_Modifiers modifiers;
	GFX_EventType type;
	GFX_Key key;
	f32 pos[2];
	f32 delta[2];
	u32 repeat_count;
	b8 is_repeat;
	b8 right_sided;
} GFX_Event;

typedef struct GFX_EventList {
	GFX_Event *first;
	GFX_Event *last;
	u64 count;
} GFX_EventList;

typedef struct GFX_PixelBuffer {
	u32 *pixels;
	s32 width;
	s32 height;
} GFX_PixelBuffer;

typedef struct GFX_DisplayParams {
	s32 dest_x;
	s32 dest_y;
	s32 dest_width;
	s32 dest_height;
} GFX_DisplayParams;

void gfx_init(void);
void gfx_quit(void);

GFX_Handle gfx_open_window(s32 width, s32 height, Str8 title);
void gfx_close_window(GFX_Handle window);
GFX_EventList gfx_get_events(void);

void gfx_blit_buffer_to_window(GFX_Handle window, GFX_PixelBuffer *buffer);

/* OpenGL */

void gfx_ogl_init(void);
void gfx_ogl_select_window(GFX_Handle window);
void gfx_ogl_window_swap(GFX_Handle window);
VoidProc *gfx_ogl_load_procedure(char *name);

#endif // GFX_H
