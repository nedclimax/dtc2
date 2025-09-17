# Dangerous Threads Crew 2.0

Original by Casey Muratori: https://github.com/cmuratori/dtc

This is a very simple example program to illustrate how you move CreateWindow/DestroyWindow onto a separate thread if you need your API architecture to allow the main thread to create and destroy windows in an arbitrary way.

It may seem like a strange thing to do, but unfortunately Win32 internals have some really bad behavior during sizing and dragging windows that make it difficult for real-time rendering applications to properly continue what they're doing during those events without interruption.  Moving CreateWindow/DestroyWindow onto a separate thread moves the message processing for all windows onto that separate thread, thereby preventing Windows' message processing from interrupting your main threads.

\- Casey

This example is slightly more complex as it aims to wrap the example code in an API not disimilar to any other library like SDL or GLFW.

The API for creating/destroying windows looks like this:

```c
void gfx_init(void);
void gfx_quit(void);

GFX_Handle gfx_open_window(s32 width, s32 height, Str8 title);
void gfx_close_window(GFX_Handle window);

// Gathers every event that has happend since
// the last time this function was called
GFX_EventList gfx_get_events(void);
```

And the usage code looks roughly like this:

```c
gfx_init();
s32 window_width = 1280;
s32 window_height = 720;
GFX_Handle window = gfx_open_window(window_width, window_height, str8("GFX Test"));
b8 running = 1;
while (running) {
	GFX_EventList events = gfx_get_events();
	for (GFX_Event *event = events.first; event; event = event->next) {
		// handle events here...
	}
	// run app logic here...
}
gfx_quit();

```