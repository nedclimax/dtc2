#include "basic.h"
#include "gfx.h"

#include "basic.c"
#include "gfx.c"

void draw_to_buffer(GFX_PixelBuffer *buffer) {
	static s32 x_offset = 0;
	static s32 y_offset = 0;
	u8 *row = (u8 *)buffer->pixels;
	s32 pitch = buffer->width * sizeof(*buffer->pixels);
	for (int y = 0; y < buffer->height; y++) {
		u32 *pixel = (u32 *)row;
		for (int x = 0; x < buffer->width; x++) {
			u8 r = 0;
			u8 g = (u8)(x + x_offset);
			u8 b = (u8)(y + y_offset);
			*pixel++ = (r << 16) | (g << 8) | (b);
		}
		row += pitch;
	}
	x_offset++;
	y_offset++;
}

GFX_PixelBuffer alloc_pixel_buffer(Arena *arena, s32 width, s32 height) {
	GFX_PixelBuffer result = {
		.pixels = push_array(arena, u32, width * height),
		.width = width,
		.height = height,
	};
	return result;
}

int entry_point(CmdLine *cmd) {
	(void)cmd;
	gfx_init();

	Arena *perm_arena = arena_alloc();
	s32 window_width = 1280;
	s32 window_height = 720;
	GFX_Handle window = gfx_open_window(window_width, window_height, str8("GFX Test"));
	GFX_PixelBuffer buffer = alloc_pixel_buffer(perm_arena, window_width, window_height);

	b8 running = 1;
	while (running) {
		GFX_EventList events = gfx_get_events();
		for (GFX_Event *event = events.first; event; event = event->next) {
			if (event->type == GFX_EventType_WindowClose) {
				running = 0;
			}
		}
		draw_to_buffer(&buffer);
		gfx_blit_buffer_to_window(window, &buffer);
	}

	arena_release(perm_arena);
	gfx_quit();
	return 0;
}
