#ifndef BASIC_H
#define BASIC_H

/* Helper Macros */

#define Min(a,b) ((a)<(b)?(a):(b))
#define Max(a,b) ((a)>(b)?(a):(b))
#define Clamp(x, min, max) Min(Max((x),(min)),(max))
#define AlignPow2(x,b)     (((x) + (b) - 1) & (~((b) - 1)))
#define AlignDownPow2(x,b) ((x) & (~((b) - 1)))
#define AlignPadPow2(x,b)  ((0 - (x)) & ((b) - 1))
#define IsPow2OrZero(x)    ((((x) - 1) & (x)) == 0)
#define IsPow2(x)          ((x) != 0 && IsPow2OrZero(x))
#define ArrayCount(arr)    (sizeof(arr)/sizeof((arr)[0]))
#define Glue_(a, b) a##b
#define Glue(a, b) Glue_(a, b)
#define Str_(x) #x
#define Str(x) Str_(x)

#define KB(n) ((n) << 10)
#define MB(n) (KB(n) << 10)
#define GB(n) (MB(n) << 10)
#define TB(n) (GB(n) << 10)

/* Linked List Building Macros */

// linked list macro helpers
#define CheckNil(nil,p) ((p) == 0 || (p) == nil)
#define SetNil(nil,p) ((p) = nil)

// doubly-linked-lists
#define DLLInsert_NPZ(nil,f,l,p,n,next,prev)                               \
	(CheckNil(nil,f) ?                                                       \
	 ((f) = (l) = (n), SetNil(nil,(n)->next), SetNil(nil,(n)->prev)) :       \
	 CheckNil(nil,p) ?                                                       \
	 ((n)->next = (f), (f)->prev = (n), (f) = (n), SetNil(nil,(n)->prev)) :  \
	 ((p)==(l)) ?                                                            \
	 ((l)->next = (n), (n)->prev = (l), (l) = (n), SetNil(nil, (n)->next)) : \
	 (((!CheckNil(nil,p) && CheckNil(nil,(p)->next)) ?                       \
	   (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define DLLPushBack_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,f,l,l,n,next,prev)
#define DLLPushFront_NPZ(nil,f,l,n,next,prev) DLLInsert_NPZ(nil,l,f,f,n,prev,next)
#define DLLRemove_NPZ(nil,f,l,n,next,prev) \
	(((n) == (f) ? (f) = (n)->next : (0)),   \
	 ((n) == (l) ? (l) = (l)->prev : (0)),   \
	 (CheckNil(nil,(n)->prev) ? (0) :        \
	  ((n)->prev->next = (n)->next)),        \
	 (CheckNil(nil,(n)->next) ? (0) :        \
	  ((n)->next->prev = (n)->prev)))

// singly-linked, doubly-headed lists (queues)
#define SLLQueuePush_NZ(nil,f,l,n,next) \
	(CheckNil(nil,f)?                     \
	 ((f)=(l)=(n),SetNil(nil,(n)->next)): \
	 ((l)->next=(n),(l)=(n),SetNil(nil,(n)->next)))
#define SLLQueuePushFront_NZ(nil,f,l,n,next) \
	(CheckNil(nil,f)?                          \
	 ((f)=(l)=(n),SetNil(nil,(n)->next)):      \
	 ((n)->next=(f),(f)=(n)))
#define SLLQueuePop_NZ(nil,f,l,next) \
	((f)==(l)?                         \
	 (SetNil(nil,f),SetNil(nil,l)):    \
	 ((f)=(f)->next))

// singly-linked, singly-headed lists (stacks)
#define SLLStackPush_N(f,n,next) ((n)->next=(f), (f)=(n))
#define SLLStackPop_N(f,next) ((f)=(f)->next)

// doubly-linked-list helpers
#define DLLInsert_NP(f,l,p,n,next,prev) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack_NP(f,l,n,next,prev) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront_NP(f,l,n,next,prev) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove_NP(f,l,n,next,prev) DLLRemove_NPZ(0,f,l,n,next,prev)
#define DLLInsert(f,l,p,n) DLLInsert_NPZ(0,f,l,p,n,next,prev)
#define DLLPushBack(f,l,n) DLLPushBack_NPZ(0,f,l,n,next,prev)
#define DLLPushFront(f,l,n) DLLPushFront_NPZ(0,f,l,n,next,prev)
#define DLLRemove(f,l,n) DLLRemove_NPZ(0,f,l,n,next,prev)

// singly-linked, doubly-headed list helpers
#define SLLQueuePush_N(f,l,n,next) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront_N(f,l,n,next) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop_N(f,l,next) SLLQueuePop_NZ(0,f,l,next)
#define SLLQueuePush(f,l,n) SLLQueuePush_NZ(0,f,l,n,next)
#define SLLQueuePushFront(f,l,n) SLLQueuePushFront_NZ(0,f,l,n,next)
#define SLLQueuePop(f,l) SLLQueuePop_NZ(0,f,l,next)

// singly-linked, singly-headed list helpers
#define SLLStackPush(f,n) SLLStackPush_N(f,n,next)
#define SLLStackPop(f) SLLStackPop_N(f,next)

#if defined(__clang__)
# define AlignOf(T) __alignof(T)
#elif defined(_MSC_VER)
# define AlignOf(T) __alignof(T)
#elif defined(__GNUC__)
# define AlignOf(T) __alignof__(T)
#else
# define AlignOf(T) _Alignof(T)
#endif

#if defined(__clang__)
# define Trap() __builtin_debugtrap()
#elif defined(_MSC_VER)
# define Trap() __debugbreak()
#elif defined(__GNUC__)
# define Trap() __asm__ volatile("int $0x03")
#else
# define Trap() ((volatile char *)0) = 0
#endif

#define Assert(cond) do { if (!(cond)) Trap(); } while(0)
#define StaticAssert(cond, id) typedef u8 Glue(static_assert__, id)[(cond)?1:-1];

#define Todo(msg) Trap()
#define InvalidPath(msg) Trap()

#if defined(__clang__) || defined(__GNUC__)
#define thread_static __thread
#elif defined(_MSC_VER)
#define thread_static __declspec(thread)
#else
#define thread_static _Thread_local
#endif

/* Standard Includes */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

/* Basic Types */

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef u8 b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;
typedef float f32;
typedef double f64;
typedef void VoidProc(void);

#define U64_MAX 0xffffffffffffffffull
#define U32_MAX 0xffffffff
#define U16_MAX 0xffff
#define U8_MAX  0xff

#define S64_MAX ((s64)0x7fffffffffffffffll)
#define S32_MAX ((s32)0x7fffffff)
#define S16_MAX ((s16)0x7fff)
#define S8_MAX  ((s8) 0x7f)

#define S64_MIN ((s64)0x8000000000000000ll)
#define S32_MIN ((s32)0x80000000)
#define S16_MIN ((s16)0x8000)
#define S8_MIN  ((s8) 0x80)

/* Arenas */

#define ARENA_HEADER_SIZE 128

typedef u64 ArenaFlags; enum {
	ArenaFlags_NoChain = (1 << 0),
};

typedef struct ArenaParams {
	u64 commit_size;
	u64 reserve_size;
	ArenaFlags flags;
	void *optional_backing_buffer;
	char *allocation_site_file;
	int allocation_site_line;
} ArenaParams;

typedef struct Arena {
	ArenaFlags flags;
	struct Arena *prev;
	struct Arena *current;
	u64 base_pos;
	u64 cmt_size;
	u64 res_size;
	u64 pos;
	u64 cmt;
	u64 res;
	char *allocation_site_file;
	int allocation_site_line;
} Arena;
StaticAssert(sizeof(Arena) <= ARENA_HEADER_SIZE, arena_header_size_check);

typedef struct Temp {
	Arena *arena;
	u64 pos;
} Temp;

Arena *arena_alloc_params(ArenaParams *params);
#define arena_alloc(...) arena_alloc_params(&(ArenaParams){\
	.commit_size = KB(64), \
	.reserve_size = MB(64), \
	.allocation_site_file = __FILE__, \
	.allocation_site_line = __LINE__, \
	__VA_ARGS__})
void arena_release(Arena *arena);

u64   arena_pos(Arena *arena);
void *arena_push(Arena *arena, u64 size, u64 align);
void  arena_pop_to(Arena *arena, u64 pos);

void arena_clear(Arena *arena);
void arena_pop(Arena *arena, u64 amt);

Temp temp_begin(Arena *arena);
void temp_end(Temp temp);

#define push_array_no_zero_aligned(a, T, c, align) (T *)arena_push((a), sizeof(T)*(c), (align))
#define push_array_aligned(a, T, c, align) (T *)memset(push_array_no_zero_aligned(a, T, c, align), 0, (sizeof(T)*(c)))
#define push_array_no_zero(a, T, c) push_array_no_zero_aligned(a, T, c, Max(8, AlignOf(T)))
#define push_array(a, T, c) push_array_aligned(a, T, c, Max(8, AlignOf(T)))
#define push_struct_no_zero(a, T) push_array_no_zero(a, T, 1)
#define push_struct(a, T) push_array(a, T, 1)

Arena *get_scratch_arena(Arena **conflicts, u64 count);
#define scratch_begin(conflicts, count) temp_begin(get_scratch_arena((conflicts), (count)))
#define scratch_end(scratch) temp_end((scratch))

/* Strings */

u64 cstr8_len(u8 *str);
u64 cstr16_len(u16 *str);

typedef struct Str8 {
	u8 *str;
	u64 size;
} Str8;

typedef struct Str8Node {
	struct Str8Node *next;
	Str8 str;
} Str8Node;

typedef struct Str8List {
	Str8Node *first;
	Str8Node *last;
	u64 node_count;
	u64 total_size;
} Str8List;

typedef struct StrJoin {
	Str8 pre;
	Str8 sep;
	Str8 post;
} StrJoin;

#define str8(s) (Str8){(u8 *)(s), sizeof(s)-1}
#define str8n(s,n) (Str8){(s), (n)}
#define str8c(cstr) (Str8){(cstr), cstr8_len((cstr))}
#define str8_range(first,opl) (Str8){(first), (u64)((opl) - (first))}
Str8 str8_skip(Str8 str, u64 amt);
Str8 str8_chop(Str8 str, u64 amt);
Str8 str8_prefix(Str8 str, u64 size);
Str8 str8_postfix(Str8 str, u64 size);
Str8 str8_skip_chop_whitespace(Str8 str);
Str8 str8_substr(Str8 str, u64 min, u64 max);
Str8 str8f(Arena *arena, char *fmt, ...);
Str8 str8fv(Arena *arena, char *fmt, va_list args);
Str8 str8_cat(Arena *arena, Str8 str1, Str8 str2);
Str8 str8_copy(Arena *arena, Str8 str);
Str8Node *str8_list_push(Arena *arena, Str8List *list, Str8 str);
Str8List str8_split(Arena *arena, Str8 str, Str8 split_chars);
Str8 str8_list_join(Arena *arena, Str8List *list, StrJoin *optional_params);

typedef u32 StrMatchFlags; enum {
  StrMatchFlags_CaseInsensitive  = (1 << 0),
  StrMatchFlags_RightSideSloppy  = (1 << 1),
  StrMatchFlags_SlashInsensitive = (1 << 2),
};
b8 str8_match(Str8 a, Str8 b, StrMatchFlags flags);
u64 str8_find_needle(Str8 str, u64 start_pos, Str8 needle, StrMatchFlags flags);
u64 str8_find_needle_reverse(Str8 str, u64 start_pos, Str8 needle, StrMatchFlags flags);

s64 sign_from_str8(Str8 str, Str8 *tail);
b8 str8_is_int(Str8 str, u8 radix);

Str8 str8_from_u64(Arena *arena, u64 u64, u32 radix, u8 min_digits, u8 digit_group_separator);
Str8 str8_from_s64(Arena *arena, s64 s64, u32 radix, u8 min_digits, u8 digit_group_separator);

u64 u64_from_str8(Str8 str, u32 radix);
s64 s64_from_str8(Str8 str, u32 radix);
f64 f64_from_str8(Str8 str);

/* Unicode String decoding/conversion */

typedef struct UnicodeDecode {
	u32 inc;
	u32 codepoint;
} UnicodeDecode;

UnicodeDecode utf8_decode(u8 *str, u64 max);
UnicodeDecode utf16_decode(u16 *str, u64 max);
u32 utf8_encode(u8 *str, u32 codepoint);
u32 utf16_encode(u16 *str, u32 codepoint);

/* This is here essentially for the Windows API :P */
typedef struct Str16 {
	u16 *str;
	u64 size;
} Str16;

#define str16(s) (Str16){(u16 *)(s), sizeof(s)}
#define str16n(s,n) (Str16){(s), (n)}
#define str16c(cstr) (Str16){(cstr), cstr16_len((cstr))}
Str16 str16_from_8(Arena *arena, Str8 str);
Str8 str8_from_16(Arena *arena, Str16 str);

// OS specific stuff

/* Virtual Memory Allocation */

void *mem_reserve(u64 size);
b8    mem_commit(void *ptr, u64 size);
void  mem_decommit(void *ptr, u64 size);
void  mem_release(void *ptr, u64 size);

/* File I/O */

Str8 data_from_file(Arena *arena, Str8 path);
b8 write_to_file(Str8 data, Str8 path);

/* Time */

u64 now_us(void);
void sleep_ms(u32 ms);

/* Printing */

void m_printf(char *fmt, ...);
void m_eprintf(char *fmt, ...);
void write_string_to_stdout(Str8 str);
void write_string_to_stderr(Str8 str);

// TODO: multi-threading

typedef struct CmdLineOpt {
  struct CmdLineOpt *next;
  struct CmdLineOpt *hash_next;
  u64 hash;
  Str8 str;
  Str8List value_strs;
  Str8 value_str;
} CmdLineOpt;

typedef struct CmdLineOptList {
  u64 count;
  CmdLineOpt *first;
  CmdLineOpt *last;
} CmdLineOptList;

typedef struct CmdLine {
	Str8 exe_name;
	Str8List inputs;
	CmdLineOptList opts;
	u64 opt_table_size;
	CmdLineOpt **opt_table;
	u64 argc;
	char **argv;
	Str8List raw_args;
} CmdLine;

/* NOTE: Entrypoint */
#ifndef MAIN_HANDLED
int entry_point(CmdLine *cmd);
#endif

#endif // BASIC_H
