#ifndef GFX_H
#define GFX_H

typedef struct GFX_Handle { u64 u64; } GFX_Handle;

typedef enum GFX_EventType {
	GFX_EventType_Null,
	GFX_EventType_Press,
	GFX_EventType_Release,
	GFX_EventType_MouseMove,
	GFX_EventType_WindowClose,
	GFX_EventType_WindowLoseFocus,
} GFX_EventType;

typedef struct GFX_Event {
	struct GFX_Event *next;
	struct GFX_Event *prev;
	GFX_Handle window;
	GFX_EventType type;
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
