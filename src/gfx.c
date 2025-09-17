/* Platform agnostic stuff */

static GFX_Event *gfx_event_list_push_new(Arena *arena, GFX_EventList *list, GFX_EventType type) {
	GFX_Event *event = push_struct(arena, GFX_Event);
	DLLPushBack(list->first, list->last, event);
	list->count++;
	event->type = type;
	return event;
}

/* Platform specific implementation */

#ifdef _WIN32
#include <Windows.h>

#pragma comment(lib, "gdi32.lib")

extern IMAGE_DOS_HEADER __ImageBase;
#define w32_gfx_hinstance ((HINSTANCE)(&__ImageBase))

typedef struct W32_Window {
	struct W32_Window *next;
	struct W32_Window *prev;
	HWND hwnd;
	HDC hdc;
} W32_Window;

static struct {
	Arena *event_arena;
	Arena *window_arena;
	W32_Window *first_window;
	W32_Window *last_window;
	W32_Window *free_window;
	GFX_EventList event_list;
	DWORD gfx_tid;
	// @DangerousThreadsCrew
	DWORD service_tid;
	HWND service_hwnd;
	HANDLE service_thread;
	CONDITION_VARIABLE service_cv;
	CRITICAL_SECTION service_mutex;
} w32_gfx_state;

#define w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((void*)-4)

/* NOTE: SendMessageW is synchronous where as PostMessageW is asynchronous */
/* Dangerous threads crew stuff */

#define CREATE_DANGEROUS_WINDOW (WM_USER + 0x1337)
#define DESTROY_DANGEROUS_WINDOW (WM_USER + 0x1338)

typedef struct TheBaby {
	DWORD     dwExStyle;
	LPCWSTR   lpClassName;
	LPCWSTR   lpWindowName;
	DWORD     dwStyle;
	int       X;
	int       Y;
	int       nWidth;
	int       nHeight;
	HWND      hWndParent;
	HMENU     hMenu;
	HINSTANCE hInstance;
	LPVOID    lpParam;
} TheBaby;

static LRESULT CALLBACK w32_service_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	/* NOTE(casey): This is not really a window handler per se, it's actually just
	   a remote thread call handler. Windows only really has blocking remote thread
	   calls if you register a WndProc for them, so that's what we do.
	   This handles CREATE_DANGEROUS_WINDOW and DESTROY_DANGEROUS_WINDOW, which are
	   just calls that do CreateWindow and DestroyWindow here on this thread when
	   some other thread wants that to happen.
	   @DangerousThreadsCrew
	*/
	LRESULT result = 0;
	switch (msg) {
		case CREATE_DANGEROUS_WINDOW: {
			TheBaby *baby = (TheBaby *)wparam;
			result = (LRESULT)CreateWindowExW(
				baby->dwExStyle,
				baby->lpClassName,
				baby->lpWindowName,
				baby->dwStyle,
				baby->X,
				baby->Y,
				baby->nWidth,
				baby->nHeight,
				baby->hWndParent,
				baby->hMenu,
				baby->hInstance,
				baby->lpParam);
		} break;
		case DESTROY_DANGEROUS_WINDOW:
			DestroyWindow((HWND)wparam);
			break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			result = DefWindowProcW(hwnd, msg, wparam, lparam);
	}
	return result;
}

static DWORD WINAPI w32_service_thread_proc(void *param) { // @DangerousThreadsCrew
	(void)param;
	if (raddbg_is_attached()) raddbg_thread_name("Window Service thread");

	WNDCLASSEXW wc = {
		.cbSize = sizeof(wc),
		.lpfnWndProc = &w32_service_wnd_proc,
		.hInstance = w32_gfx_hinstance,
		.lpszClassName = L"DTCClass",
	};
	RegisterClassExW(&wc);

	EnterCriticalSection(&w32_gfx_state.service_mutex);
	w32_gfx_state.service_hwnd = CreateWindowExW(
		0, wc.lpszClassName,
		L"DTCService", 0,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, wc.hInstance, NULL);
	WakeConditionVariable(&w32_gfx_state.service_cv);
	LeaveCriticalSection(&w32_gfx_state.service_mutex);

	// NOTE(casey): This thread can just idle for the rest of the run,
	// and call DispatchMessage
	for (;;) {
		MSG msg;
		GetMessageW(&msg, NULL, 0, 0);
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

static LRESULT CALLBACK w32_gfx_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	/* NOTE(casey): This is an example of an actual window procedure. It doesn't do anything
	   but forward things to the main thread, because again, all window messages now occur
	   on the message thread, and presumably we would rather handle everything there.  You
	   don't _have_ to do that - you could choose to handle some of the messages here.
	   But if you did, you would have to actually think about whether there are race conditions
	   with your main thread and all that.  So just PostThreadMessageW()'ing everything gets
	   you out of having to think about it.
	   @DangerousThreadsCrew
	*/
	LRESULT result = 0;
	switch (msg) {
		// NOTE(casey): Mildly annoying, if you want to specify a window, you have
		// to snuggle the params yourself, because Windows doesn't let you forward
		// a god damn window message even though the program IS CALLED WINDOWS. It's
		// in the name! Let me pass it!
		case WM_CLOSE:
			PostThreadMessageW(w32_gfx_state.gfx_tid, msg, (WPARAM)hwnd, lparam);
			break;
		// NOTE(casey): Anything you want the application to handle, forward to the main thread
		// here.
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			PostThreadMessageW(w32_gfx_state.gfx_tid, msg, wparam, lparam);
			break;
		default:
			result = DefWindowProcW(hwnd, msg, wparam, lparam);
	}
	return result;
}

static W32_Window *w32_window_alloc(void) {
	W32_Window *result = w32_gfx_state.free_window;
	if (result) {
		SLLStackPop(w32_gfx_state.free_window);
	} else {
		result = push_struct_no_zero(w32_gfx_state.window_arena, W32_Window);
	}
	memset(result, 0, sizeof(*result));
	if (result) {
		DLLPushBack(w32_gfx_state.first_window, w32_gfx_state.last_window, result);
	}
	return result;
}

static void w32_window_release(W32_Window *window) { // @DangerousThreadsCrew
	ReleaseDC(window->hwnd, window->hdc);
	SendMessageW(w32_gfx_state.service_hwnd, DESTROY_DANGEROUS_WINDOW, (u64)window->hwnd, 0);
	DLLRemove(w32_gfx_state.first_window, w32_gfx_state.last_window, window);
	SLLStackPush(w32_gfx_state.free_window, window);
}

static GFX_Event *w32_push_event(GFX_EventType type, W32_Window *window) {
	Arena *arena = w32_gfx_state.event_arena;
	GFX_EventList *list = &w32_gfx_state.event_list;
	GFX_Event *event = gfx_event_list_push_new(arena, list, type);
	event->window.u64 = (u64)window;
	return event;
}

static W32_Window *os_w32_window_from_hwnd(HWND hwnd) {
	W32_Window *result = 0;
	for (W32_Window *w = w32_gfx_state.first_window; w; w = w->next) {
		if (w->hwnd == hwnd) {
			result = w;
			break;
		}
	}
	return result;
}

void gfx_init(void) {
	w32_gfx_state.event_arena = arena_alloc();
	w32_gfx_state.window_arena = arena_alloc();
	w32_gfx_state.gfx_tid = GetCurrentThreadId();

	// set dpi awareness
	HMODULE user32 = GetModuleHandleA("user32.dll");
	BOOL (WINAPI *w32_SetProcessDpiAwarenessContext)(void* value) = NULL;
	*(FARPROC *)(&w32_SetProcessDpiAwarenessContext) = GetProcAddress(user32, "SetProcessDpiAwarenessContext");
	if (w32_SetProcessDpiAwarenessContext) {
		w32_SetProcessDpiAwarenessContext(w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	} else {
		SetProcessDPIAware();
	}

	InitializeCriticalSection(&w32_gfx_state.service_mutex);
	InitializeConditionVariable(&w32_gfx_state.service_cv);

	// NOTE: @DangerousThreadsCrew create window service thread
	EnterCriticalSection(&w32_gfx_state.service_mutex);
	w32_gfx_state.service_thread = CreateThread(NULL, 0, w32_service_thread_proc, NULL, 0, &w32_gfx_state.service_tid);
	SleepConditionVariableCS(&w32_gfx_state.service_cv, &w32_gfx_state.service_mutex, INFINITE);
	LeaveCriticalSection(&w32_gfx_state.service_mutex);
}

void gfx_quit(void) {
	arena_release(w32_gfx_state.event_arena);
	arena_release(w32_gfx_state.window_arena);
	SendMessageW(w32_gfx_state.service_hwnd, WM_CLOSE, 0, 0);
	DeleteCriticalSection(&w32_gfx_state.service_mutex);
	CloseHandle(w32_gfx_state.service_thread);
}

GFX_Handle gfx_open_window(s32 width, s32 height, Str8 title) { // @DangerousThreadsCrew
	static WCHAR *class_name = NULL;
	if (!class_name) {
		class_name = L"graphical-window";
		WNDCLASSEXW wc = {
			.cbSize = sizeof(wc),
			.lpfnWndProc = w32_gfx_wnd_proc,
			.hInstance = w32_gfx_hinstance,
			.lpszClassName = class_name,
			.hCursor = LoadCursorA(0, IDC_ARROW),
			.style = CS_VREDRAW|CS_HREDRAW,
		};
		RegisterClassExW(&wc);
	}

	Temp scratch = scratch_begin(NULL, 0);
	Str16 title16 = str16_from_8(scratch.arena, title);
	TheBaby baby = {
		.dwExStyle = 0,
		.lpClassName = class_name,
		.lpWindowName = (WCHAR *)title16.str,
		.dwStyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		.X = CW_USEDEFAULT,
		.Y = CW_USEDEFAULT,
		.nWidth = width,
		.nHeight = height,
		.hInstance = w32_gfx_hinstance,
	};
	HWND hwnd = (HWND)SendMessageW(w32_gfx_state.service_hwnd, CREATE_DANGEROUS_WINDOW, (WPARAM)&baby, 0);
	scratch_end(scratch);

	W32_Window *window = w32_window_alloc();
	window->hwnd = hwnd;
	window->hdc = GetDC(hwnd);
	SetStretchBltMode(window->hdc, COLORONCOLOR);
	GFX_Handle result = { (u64)window };
	return result;
}

void gfx_close_window(GFX_Handle window) { // @DangerousThreadsCrew
	w32_window_release((W32_Window *)window.u64);
}

GFX_EventList gfx_get_events(void) { // @DangerousThreadsCrew
	memset(&w32_gfx_state.event_list, 0, sizeof(w32_gfx_state.event_list));
	arena_clear(w32_gfx_state.event_arena);
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		W32_Window *window = os_w32_window_from_hwnd(msg.hwnd);
		switch (msg.message) {
			case WM_CLOSE: {
				w32_push_event(GFX_EventType_WindowClose, window);
			} break;
		}
	}
	return w32_gfx_state.event_list;
}

void gfx_blit_buffer_to_window(GFX_Handle window, GFX_PixelBuffer *buffer) {
	W32_Window *w = (W32_Window *)window.u64;
	RECT client_rect;
	GetClientRect(w->hwnd, &client_rect);
	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = buffer->width;
	bmi.bmiHeader.biHeight = -buffer->height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	StretchDIBits(
		w->hdc, client_rect.left, client_rect.top,
		client_rect.right - client_rect.left,
		client_rect.bottom - client_rect.top,
		0, 0, buffer->width, buffer->height,
		buffer->pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

/* END _WIN32 */
#else
# error Unsupported platform!!
#endif
