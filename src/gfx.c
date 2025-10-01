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
#define w32_gfx_class_name L"graphical-window"

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

static GFX_Key w32_key_from_vkey_table[] = {
	['A']           = GFX_Key_A,
	['B']           = GFX_Key_B,
	['C']           = GFX_Key_C,
	['D']           = GFX_Key_D,
	['E']           = GFX_Key_E,
	['F']           = GFX_Key_F,
	['G']           = GFX_Key_G,
	['H']           = GFX_Key_H,
	['I']           = GFX_Key_I,
	['J']           = GFX_Key_J,
	['K']           = GFX_Key_K,
	['L']           = GFX_Key_L,
	['M']           = GFX_Key_M,
	['N']           = GFX_Key_N,
	['O']           = GFX_Key_O,
	['P']           = GFX_Key_P,
	['Q']           = GFX_Key_Q,
	['R']           = GFX_Key_R,
	['S']           = GFX_Key_S,
	['T']           = GFX_Key_T,
	['U']           = GFX_Key_U,
	['V']           = GFX_Key_V,
	['W']           = GFX_Key_W,
	['X']           = GFX_Key_X,
	['Y']           = GFX_Key_Y,
	['Z']           = GFX_Key_Z,
	['0']           = GFX_Key_0,
	['1']           = GFX_Key_1,
	['2']           = GFX_Key_2,
	['3']           = GFX_Key_3,
	['4']           = GFX_Key_4,
	['5']           = GFX_Key_5,
	['6']           = GFX_Key_6,
	['7']           = GFX_Key_7,
	['8']           = GFX_Key_8,
	['9']           = GFX_Key_9,
	[VK_NUMPAD0]    = GFX_Key_0,
	[VK_NUMPAD1]    = GFX_Key_1,
	[VK_NUMPAD2]    = GFX_Key_2,
	[VK_NUMPAD3]    = GFX_Key_3,
	[VK_NUMPAD4]    = GFX_Key_4,
	[VK_NUMPAD5]    = GFX_Key_5,
	[VK_NUMPAD6]    = GFX_Key_6,
	[VK_NUMPAD7]    = GFX_Key_7,
	[VK_NUMPAD8]    = GFX_Key_8,
	[VK_NUMPAD9]    = GFX_Key_9,
	[VK_F1]         = GFX_Key_F1,
	[VK_F2]         = GFX_Key_F2,
	[VK_F3]         = GFX_Key_F3,
	[VK_F4]         = GFX_Key_F4,
	[VK_F5]         = GFX_Key_F5,
	[VK_F6]         = GFX_Key_F6,
	[VK_F7]         = GFX_Key_F7,
	[VK_F8]         = GFX_Key_F8,
	[VK_F9]         = GFX_Key_F9,
	[VK_F10]        = GFX_Key_F10,
	[VK_F11]        = GFX_Key_F11,
	[VK_F12]        = GFX_Key_F12,
	[VK_F13]        = GFX_Key_F13,
	[VK_F14]        = GFX_Key_F14,
	[VK_F15]        = GFX_Key_F15,
	[VK_F16]        = GFX_Key_F16,
	[VK_F17]        = GFX_Key_F17,
	[VK_F18]        = GFX_Key_F18,
	[VK_F19]        = GFX_Key_F19,
	[VK_F20]        = GFX_Key_F20,
	[VK_F21]        = GFX_Key_F21,
	[VK_F22]        = GFX_Key_F22,
	[VK_F23]        = GFX_Key_F23,
	[VK_F24]        = GFX_Key_F24,
	[VK_SPACE]      = GFX_Key_Space,
	[VK_OEM_3]      = GFX_Key_Tick,
	[VK_OEM_MINUS]  = GFX_Key_Minus,
	[VK_OEM_PLUS]   = GFX_Key_Equal,
	[VK_OEM_4]      = GFX_Key_LeftBracket,
	[VK_OEM_6]      = GFX_Key_RightBracket,
	[VK_OEM_1]      = GFX_Key_Semicolon,
	[VK_OEM_7]      = GFX_Key_Quote,
	[VK_OEM_COMMA]  = GFX_Key_Comma,
	[VK_OEM_PERIOD] = GFX_Key_Period,
	[VK_OEM_2]      = GFX_Key_Slash,
	[VK_OEM_5]      = GFX_Key_BackSlash,
	[VK_TAB]        = GFX_Key_Tab,
	[VK_PAUSE]      = GFX_Key_Pause,
	[VK_ESCAPE]     = GFX_Key_Esc,
	[VK_UP]         = GFX_Key_Up,
	[VK_LEFT]       = GFX_Key_Left,
	[VK_DOWN]       = GFX_Key_Down,
	[VK_RIGHT]      = GFX_Key_Right,
	[VK_BACK]       = GFX_Key_Backspace,
	[VK_RETURN]     = GFX_Key_Return,
	[VK_DELETE]     = GFX_Key_Delete,
	[VK_INSERT]     = GFX_Key_Insert,
	[VK_PRIOR]      = GFX_Key_PageUp,
	[VK_NEXT]       = GFX_Key_PageDown,
	[VK_HOME]       = GFX_Key_Home,
	[VK_END]        = GFX_Key_End,
	[VK_CAPITAL]    = GFX_Key_CapsLock,
	[VK_NUMLOCK]    = GFX_Key_NumLock,
	[VK_SCROLL]     = GFX_Key_ScrollLock,
	[VK_APPS]       = GFX_Key_Menu,
	[VK_CONTROL]    = GFX_Key_Ctrl,
	[VK_LCONTROL]   = GFX_Key_Ctrl,
	[VK_RCONTROL]   = GFX_Key_Ctrl,
	[VK_SHIFT]      = GFX_Key_Shift,
	[VK_LSHIFT]     = GFX_Key_Shift,
	[VK_RSHIFT]     = GFX_Key_Shift,
	[VK_MENU]       = GFX_Key_Alt,
	[VK_LMENU]      = GFX_Key_Alt,
	[VK_RMENU]      = GFX_Key_Alt,
	[VK_DIVIDE]     = GFX_Key_NumSlash,
	[VK_MULTIPLY]   = GFX_Key_NumStar,
	[VK_SUBTRACT]   = GFX_Key_NumMinus,
	[VK_ADD]        = GFX_Key_NumPlus,
	[VK_DECIMAL]    = GFX_Key_NumPeriod,
};

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
		.lpfnWndProc = w32_service_wnd_proc,
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
		// NOTE(casey): Anything we want the application to handle, we forward to the
		// main thread here.
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP: {
			if (wparam != VK_MENU && (wparam < VK_F1 || VK_F24 < wparam || wparam == VK_F4)) {
				result = DefWindowProcW(hwnd, msg, wparam, lparam);
			}
		} // fallthrough;
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_KILLFOCUS:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
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
	SendMessageW(w32_gfx_state.service_hwnd, DESTROY_DANGEROUS_WINDOW, (WPARAM)window->hwnd, 0);
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

static W32_Window *w32_window_from_hwnd(HWND hwnd) {
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

	// register window class
	WNDCLASSEXW wc = {
		.cbSize = sizeof(wc),
		.lpfnWndProc = w32_gfx_wnd_proc,
		.hInstance = w32_gfx_hinstance,
		.lpszClassName = w32_gfx_class_name,
		.hCursor = LoadCursorA(0, IDC_ARROW),
		.style = CS_VREDRAW|CS_HREDRAW,
	};
	RegisterClassExW(&wc);
}

void gfx_quit(void) {
	arena_release(w32_gfx_state.event_arena);
	arena_release(w32_gfx_state.window_arena);
	SendMessageW(w32_gfx_state.service_hwnd, WM_CLOSE, 0, 0);
	DeleteCriticalSection(&w32_gfx_state.service_mutex);
	CloseHandle(w32_gfx_state.service_thread);
}

GFX_Handle gfx_open_window(s32 width, s32 height, Str8 title) { // @DangerousThreadsCrew
	Temp scratch = scratch_begin(NULL, 0);
	Str16 title16 = str16_from_8(scratch.arena, title);
	TheBaby baby = {
		.dwExStyle = 0,
		.lpClassName = w32_gfx_class_name,
		.lpWindowName = (WCHAR *)title16.str,
		.dwStyle = WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		.X = CW_USEDEFAULT,
		.Y = CW_USEDEFAULT,
		.nWidth = width,
		.nHeight = height,
		.hInstance = w32_gfx_hinstance,
	};
	HWND hwnd = (HWND)SendMessageW(w32_gfx_state.service_hwnd, CREATE_DANGEROUS_WINDOW, (WPARAM)&baby, 0);
	W32_Window *window = w32_window_alloc();
	window->hwnd = hwnd;
	window->hdc = GetDC(hwnd);
	SetStretchBltMode(window->hdc, COLORONCOLOR);
	scratch_end(scratch);
	GFX_Handle result = { (u64)window };
	return result;
}

void gfx_close_window(GFX_Handle window) { // @DangerousThreadsCrew
	w32_window_release((W32_Window *)window.u64);
}

GFX_EventList gfx_get_events(void) { // @DangerousThreadsCrew
	arena_clear(w32_gfx_state.event_arena);
	memset(&w32_gfx_state.event_list, 0, sizeof(w32_gfx_state.event_list));

	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		b8 release = 0;
		W32_Window *window = w32_window_from_hwnd(msg.hwnd);

		switch (msg.message) {
			case WM_CLOSE: {
				W32_Window *w = (W32_Window *)msg.wParam; // NOTE: see `w32_gfx_wnd_proc`
				w32_push_event(GFX_EventType_WindowClose, w);
			} break;

			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP: {
				release = 1;
			} // fallthrough;
			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN: {
				GFX_Event *event = w32_push_event(release ? GFX_EventType_Release : GFX_EventType_Press, window);
				switch (msg.message) {
					case WM_LBUTTONUP: case WM_LBUTTONDOWN: event->key = GFX_Key_LeftMouseButton; break;
					case WM_MBUTTONUP: case WM_MBUTTONDOWN: event->key = GFX_Key_MiddleMouseButton; break;
					case WM_RBUTTONUP: case WM_RBUTTONDOWN: event->key = GFX_Key_RightMouseButton; break;
				}
				event->pos[0] = (f32)(s16)LOWORD(msg.lParam);
				event->pos[1] = (f32)(s16)HIWORD(msg.lParam);
				if (release) {
					ReleaseCapture();
				} else {
					SetCapture(msg.hwnd);
				}
			} break;

			case WM_MOUSEMOVE: {
				GFX_Event *event = w32_push_event(GFX_EventType_MouseMove, window);
				event->pos[0] = (f32)(s16)LOWORD(msg.lParam);
				event->pos[1] = (f32)(s16)HIWORD(msg.lParam);
			} break;

			case WM_MOUSEWHEEL: {
				s16 wheel_delta = HIWORD(msg.wParam);
				GFX_Event *event = w32_push_event(GFX_EventType_Scroll, window);
				POINT p;
				p.x = (s32)(s16)LOWORD(msg.lParam);
				p.y = (s32)(s16)HIWORD(msg.lParam);
				ScreenToClient(window->hwnd, &p);
				event->pos[0] = (f32)p.x;
				event->pos[1] = (f32)p.y;
				event->delta[0] = 0.f;
				event->delta[1] = -(f32)wheel_delta;
			} break;

			case WM_MOUSEHWHEEL: {
				s16 wheel_delta = HIWORD(msg.wParam);
				GFX_Event *event = w32_push_event(GFX_EventType_Scroll, window);
				POINT p;
				p.x = (s32)(s16)LOWORD(msg.lParam);
				p.y = (s32)(s16)HIWORD(msg.lParam);
				ScreenToClient(window->hwnd, &p);
				event->pos[0] = (f32)p.x;
				event->pos[1] = (f32)p.y;
				event->delta[0] = (f32)wheel_delta;
				event->delta[1] = 0.f;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP: {
				b32 was_down = (msg.lParam & (1 << 30));
				b32 is_down  = !(msg.lParam & (1 << 31));

				b8 is_repeat = 0;
				if (!is_down) {
					release = 1;
				} else if (was_down) {
					is_repeat = 1;
				}

				b8 right_sided = 0;
				if ((msg.lParam & (1 << 24)) &&
				    (msg.wParam == VK_CONTROL || msg.wParam == VK_RCONTROL ||
				     msg.wParam == VK_MENU || msg.wParam == VK_RMENU ||
				     msg.wParam == VK_SHIFT || msg.wParam == VK_RSHIFT)) {
					right_sided = 1;
				}

				GFX_Event *event = w32_push_event(release ? GFX_EventType_Release : GFX_EventType_Press, window);
				event->key = w32_key_from_vkey_table[msg.wParam];
				event->repeat_count = msg.lParam & 0xffff;
				event->is_repeat = is_repeat;
				event->right_sided = right_sided;
				if (event->key == GFX_Key_Alt   && event->modifiers & GFX_Modifier_Alt)   event->modifiers &= ~GFX_Modifier_Alt;
				if (event->key == GFX_Key_Ctrl  && event->modifiers & GFX_Modifier_Ctrl)  event->modifiers &= ~GFX_Modifier_Ctrl;
				if (event->key == GFX_Key_Shift && event->modifiers & GFX_Modifier_Shift) event->modifiers &= ~GFX_Modifier_Shift;
			} break;

			case WM_KILLFOCUS: {
				w32_push_event(GFX_EventType_WindowLoseFocus, window);
				ReleaseCapture();
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
