/* Third party includes */

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#include "stb_sprintf.h"

#define RADDBG_MARKUP_VSNPRINTF stbsp_vsnprintf
#define RADDBG_MARKUP_IMPLEMENTATION
#ifndef _WIN32
#define RADDBG_MARKUP_STUBS
#endif
#include "raddbg_markup.h"

#define XXH_NO_STREAM
#if defined(__x86_64__) || defined(_M_AMD64)
# include "xxh_x86dispatch.h"
#else
# define XXH_IMPLEMENTATION
# include "xxhash.h"
#endif

/* Arenas */

static thread_static struct {
	Arena *arenas[2];
	b8 initialized;
} tctx;

u64 get_page_size(void); // TODO: Should we put this in `basic.h` ?

Arena *get_scratch_arena(Arena **conflicts, u64 count) {
	if (!tctx.initialized) {
		tctx.arenas[0] = arena_alloc();
		tctx.arenas[1] = arena_alloc();
		tctx.initialized = 1;
	}
	Arena *result = 0;
	Arena **arena_ptr = tctx.arenas;
	for (u64 i = 0; i < ArrayCount(tctx.arenas); i++, arena_ptr++) {
		Arena **conflict_ptr = conflicts;
		b8 has_conflict = 0;
		for (u64 j = 0; j < count; j++, conflict_ptr++) {
			if (*arena_ptr == *conflict_ptr) {
				has_conflict = 1;
				break;
			}
		}
		if (!has_conflict) {
			result = *arena_ptr;
			break;
		}
	}
	return result;
}

Arena *arena_alloc_params(ArenaParams *params) {
	// round up reserve/commit sizes
	u64 reserve_size = params->reserve_size;
	u64 commit_size = params->commit_size;
	reserve_size = AlignPow2(reserve_size, get_page_size());
	commit_size  = AlignPow2(commit_size,  get_page_size());

	// reserve/commit initial block
	void *base = params->optional_backing_buffer;
	if (!base) {
		base = mem_reserve(reserve_size);
		mem_commit(base, commit_size);
		raddbg_annotate_vaddr_range(base, reserve_size, "arena %s:%i", params->allocation_site_file, params->allocation_site_line);
	}

	// panic on arena creation failure
	Assert(base);

	// extract arena header & fill
	Arena *arena = (Arena *)base;
	arena->current = arena;
	arena->flags = params->flags;
	arena->cmt_size = params->commit_size;
	arena->res_size = params->reserve_size;
	arena->base_pos = 0;
	arena->pos = ARENA_HEADER_SIZE;
	arena->cmt = commit_size;
	arena->res = reserve_size;
	arena->allocation_site_file = params->allocation_site_file;
	arena->allocation_site_line = params->allocation_site_line;
	return arena;
}

void arena_release(Arena *arena) {
	for (Arena *n = arena->current, *prev = 0; n; n = prev) {
		prev = n->prev;
		mem_release(n, n->res);
	}
}

void *arena_push(Arena *arena, u64 size, u64 align) {
	Arena *current = arena->current;
	u64 pos_pre = AlignPow2(current->pos, align);
	u64 pos_pst = pos_pre + size;

	// chain, if needed
	if (current->res < pos_pst && !(arena->flags & ArenaFlags_NoChain)) {
		Arena *new_block = 0;
		if (new_block == 0) {
			u64 res_size = current->res_size;
			u64 cmt_size = current->cmt_size;
			if (size + ARENA_HEADER_SIZE > res_size) {
				res_size = AlignPow2(size + ARENA_HEADER_SIZE, align);
				cmt_size = AlignPow2(size + ARENA_HEADER_SIZE, align);
			}
			new_block = arena_alloc(.reserve_size = res_size,
			                        .commit_size  = cmt_size,
			                        .flags        = current->flags,
			                        .allocation_site_file = current->allocation_site_file,
			                        .allocation_site_line = current->allocation_site_line);
		}

		new_block->base_pos = current->base_pos + current->res;
		SLLStackPush_N(arena->current, new_block, prev);

		current = new_block;
		pos_pre = AlignPow2(current->pos, align);
		pos_pst = pos_pre + size;
	}

	// commit new pages, if needed
	if (current->cmt < pos_pst) {
		u64 cmt_pst_aligned = pos_pst + current->cmt_size-1;
		cmt_pst_aligned -= cmt_pst_aligned%current->cmt_size;
		u64 cmt_pst_clamped = Min(cmt_pst_aligned, current->res);
		u64 cmt_size = cmt_pst_clamped - current->cmt;
		u8 *cmt_ptr = (u8 *)current + current->cmt;
		mem_commit(cmt_ptr, cmt_size);
		current->cmt = cmt_pst_clamped;
	}

	// push onto current block
	void *result = 0;
	if (current->cmt >= pos_pst) {
		result = (u8 *)current + pos_pre;
		current->pos = pos_pst;
	}

	// panic on failure
	Assert(result);
	return result;
}

u64 arena_pos(Arena *arena) {
	Arena *current = arena->current;
	u64 pos = current->base_pos + current->pos;
	return pos;
}

void arena_pop_to(Arena *arena, u64 pos) {
	u64 big_pos = Max(ARENA_HEADER_SIZE, pos);
	Arena *current = arena->current;

	for (Arena *prev = 0; current->base_pos >= big_pos; current = prev) {
		prev = current->prev;
		mem_release(current, current->res);
	}

	arena->current = current;
	u64 new_pos = big_pos - current->base_pos;
	Assert(new_pos <= current->pos);
	current->pos = new_pos;
}

void arena_clear(Arena *arena) {
	arena_pop_to(arena, 0);
}

void arena_pop(Arena *arena, u64 amt) {
	u64 pos_old = arena_pos(arena);
	u64 pos_new = pos_old;
	if (amt < pos_old) {
		pos_new = pos_old - amt;
	}
	arena_pop_to(arena, pos_new);
}

Temp temp_begin(Arena *arena) {
	u64 pos = arena_pos(arena);
	Temp temp = {arena, pos};
	return temp;
}

void temp_end(Temp temp) {
	arena_pop_to(temp.arena, temp.pos);
}

/* Strings */

u64 cstr8_len(u8 *str) {
	u64 i = 0;
	while (*str++) i++;
	return i;
}

u64 cstr16_len(u16 *str) {
	u64 i = 0;
	while (*str++) i++;
	return i;
}

u16 safe_cast_u16(u32 x) {
	Assert(x <= U16_MAX);
	return (u16)x;
}

u32 safe_cast_u32(u64 x) {
	Assert(x <= U32_MAX);
	return (u32)x;
}

s32 safe_cast_s32(s64 x) {
	Assert(S32_MIN <= x && x <= S32_MAX);
	return (s32)x;
}

static u8 integer_symbols[16] = {
	'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
};

// NOTE: Includes reverses for uppercase and lowercase hex.
static u8 integer_symbol_reverse[128] = {
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

b8 char_is_space(u8 c) {
	return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t') || (c == '\v') || (c == '\f');
}

b8 char_is_upper(u8 c) {
	return ('A' <= c && c <= 'Z');
}

b8 char_is_lower(u8 c) {
	return ('a' <= c && c <= 'z');
}

b8 char_is_alpha(u8 c) {
	return char_is_upper(c) || char_is_lower(c);
}

b8 char_is_slash(u8 c) {
	return (c == '/') || (c == '\\');
}

b8 char_is_digit(u8 c, u32 base) {
	return ((0 < base && base <= 16) && (integer_symbol_reverse[c] < base));
}

u8 char_to_lower(u8 c) {
	return char_is_upper(c) ? c + ('a' - 'A') : c;
}

u8 char_to_upper(u8 c) {
	return char_is_lower(c) ? c - ('a' - 'A') : c;
}

u8 char_to_correct_slash(u8 c) {
	return char_is_slash(c) ? '/' : c;
}

Str8 str8_substr(Str8 str, u64 min, u64 max) {
	min = Min(min, str.size);
	max = Min(max, str.size);
	str.str += min;
	str.size = max - min;
	return str;
}

Str8 str8_prefix(Str8 str, u64 size) {
	str.size = Min(size, str.size);
	return str;
}

Str8 str8_skip(Str8 str, u64 amt) {
	amt = Min(amt, str.size);
	str.str += amt;
	str.size -= amt;
	return str;
}

Str8 str8_postfix(Str8 str, u64 size) {
	size = Min(size, str.size);
	str.str = (str.str + str.size) - size;
	str.size = size;
	return str;
}

Str8 str8_chop(Str8 str, u64 amt) {
	amt = Min(amt, str.size);
	str.size += amt;
	return str;
}

Str8 str8_skip_chop_whitespace(Str8 str) {
	u64 i = 0;
	u64 j = str.size;
	for (; i < str.size; i++) {
		if (!char_is_space(str.str[i])) {
			break;
		}
	}
	while (j > i) {
		j--;
		if (!char_is_space(str.str[j])) {
			j++;
			break;
		}
	}
	Str8 result = { str.str + i, j - i};
	return result;
}

Str8 str8_copy(Arena *arena, Str8 str) {
	Str8 result;
	result.size = str.size;
	result.str = push_array_no_zero(arena, u8, result.size + 1);
	memmove(result.str, str.str, str.size);
	result.str[result.size] = 0;
	return result;
}

Str8 str8_cat(Arena *arena, Str8 str1, Str8 str2) {
	Str8 result;
	result.size = str1.size + str2.size;
	result.str = push_array_no_zero(arena, u8, result.size + 1);
	memmove(result.str, str1.str, str1.size);
	memmove(result.str + str1.size, str2.str, str2.size);
	result.str[result.size] = 0;
	return result;
}

Str8 str8f(Arena *arena, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	Str8 result = str8fv(arena, fmt, args);
	va_end(args);
	return result;
}

Str8 str8fv(Arena *arena, char *fmt, va_list args) {
	va_list args2;
	va_copy(args2, args);
	u32 needed_bytes = stbsp_vsnprintf(NULL, 0, fmt, args) + 1;
	Str8 result;
	result.str = push_array_no_zero(arena, u8, needed_bytes);
	result.size = stbsp_vsnprintf((char *)result.str, needed_bytes, fmt, args2);
	result.str[result.size] = 0;
	va_end(args2);
	return result;
}

Str8Node *str8_list_push(Arena *arena, Str8List *list, Str8 str) {
	Str8Node *node = push_struct_no_zero(arena, Str8Node);
	SLLQueuePush(list->first, list->last, node);
	list->node_count++;
	list->total_size += str.size;
	node->str = str;
	return node;
}

Str8List str8_split(Arena *arena, Str8 str, Str8 split_chars) {
	Str8List list = {0};
	for (u64 i = 0; i < str.size; i++) {
		u64 first = i;
		for (; i < str.size; i++) {
			u8 c = str.str[i];
			b8 is_split = 0;
			for (u64 j = 0; j < split_chars.size; j++) {
				if (split_chars.str[j] == c) {
					is_split = 1;
					break;
				}
			}
			if (is_split) break;
		}
		Str8 res = { str.str + first, i - first };
		if (res.size > 0) str8_list_push(arena, &list, res);
	}
	return list;
}

Str8 str8_list_join(Arena *arena, Str8List *list, StrJoin *optional_params) {
	StrJoin join = {0};
	if (optional_params) join = *optional_params;
	u64 sep_count = list->node_count > 0 ? list->node_count - 1 : 0;
	Str8 result;
	result.size = join.pre.size + join.post.size + sep_count * join.sep.size + list->total_size;
	u8 *ptr = result.str = push_array_no_zero(arena, u8, result.size + 1);
	memmove(ptr, join.pre.str, join.pre.size);
	ptr += join.pre.size;
	for (Str8Node *n = list->first; n; n = n->next) {
		memmove(ptr, n->str.str, n->str.size);
		ptr += n->str.size;
		if (n->next) {
			memmove(ptr, join.sep.str, join.sep.size);
			ptr += join.sep.size;
		}
	}
	memmove(ptr, join.post.str, join.post.size);
	ptr += join.post.size;
	*ptr = 0;
	return result;
}

b8 str8_match(Str8 a, Str8 b, StrMatchFlags flags) {
	b8 result = 0;
	if (a.size == b.size && !flags) {
		result = memcmp(a.str, b.str, b.size) == 0;
	} else if (a.size == b.size || (flags & StrMatchFlags_RightSideSloppy)) {
		b32 case_insensitive = (flags & StrMatchFlags_CaseInsensitive);
		b32 slash_insensitive = (flags & StrMatchFlags_SlashInsensitive);
		u64 size = Min(a.size, b.size);
		result = 1;
		for (u64 i = 0; i < size; i++) {
			u8 at = a.str[i];
			u8 bt = b.str[i];
			if (case_insensitive) {
				at = char_to_upper(at);
				bt = char_to_upper(bt);
			}
			if (slash_insensitive) {
				at = char_to_correct_slash(at);
				bt = char_to_correct_slash(bt);
			}
			if (at != bt) {
				result = 0;
				break;
			}
		}
	}
	return result;
}

u64 str8_find_needle(Str8 str, u64 start_pos, Str8 needle, StrMatchFlags flags) {
	u8 *p = str.str + start_pos;
	u64 stop_offset = Max(str.size + 1, needle.size) - needle.size;
	u8 *stop_p = str.str + stop_offset;
	if (needle.size > 0) {
		u8 *str_opl = str.str + str.size;
		Str8 needle_tail = str8_skip(needle, 1);
		StrMatchFlags adjusted_flags = flags | StrMatchFlags_RightSideSloppy;
		u8 needle_first_char_adjusted = needle.str[0];
		if (adjusted_flags & StrMatchFlags_CaseInsensitive) {
			needle_first_char_adjusted = char_to_upper(needle_first_char_adjusted);
		}
		for (;p < stop_p; p++) {
			u8 haystack_char_adjusted = *p;
			if (adjusted_flags & StrMatchFlags_CaseInsensitive)
				haystack_char_adjusted = char_to_upper(haystack_char_adjusted);
			if (haystack_char_adjusted == needle_first_char_adjusted &&
			    str8_match(str8_range(p + 1, str_opl), needle_tail, adjusted_flags)) break;
		}
	}
	u64 result = str.size;
	if (p < stop_p) result = (u64)(p - str.str);
	return result;
}

u64 str8_find_needle_reverse(Str8 str, u64 start_pos, Str8 needle, StrMatchFlags flags);

s64 sign_from_str8(Str8 str, Str8 *str_tail) {
	// count negative signs
	u64 neg_count = 0;
	u64 i = 0;
	for (; i < str.size; i++) {
		if (str.str[i] == '-') {
			neg_count++;
		} else if (str.str[i] != '+') {
			break;
		}
	}

	// output part of string after signs
	if (str_tail) *str_tail = str8_skip(str, i);

	// output integer sign
	u64 sign = (neg_count & 1) ? -1 : +1;
	return sign;
}

b8 str8_is_int(Str8 str, u8 radix) {
	if (str.size == 0) return 0;
	if (radix <= 1 || 16 < radix) return 0;
	b8 result = 1;
	for (u64 i = 0; i < str.size; i++) {
		u8 c = str.str[i];
		if (!(c < 0x80) || integer_symbol_reverse[c] >= radix) {
			result = 0;
			break;
		}
	}
	return result;
}

// TODO: Implement
Str8 str8_from_u64(Arena *arena, u64 u64, u32 radix, u8 min_digits, u8 digit_group_separator);
Str8 str8_from_s64(Arena *arena, s64 s64, u32 radix, u8 min_digits, u8 digit_group_separator);

u64 u64_from_str8(Str8 str, u32 radix) {
	u64 x = 0;
	if (radix <= 1 || 16 < radix) return x;
	for (u64 i = 0; i < str.size; i++) {
		if (!char_is_digit(str.str[i], radix)) break;
		x *= radix;
		x += integer_symbol_reverse[str.str[i] & 0x7F];
	}
	return x;
}

s64 s64_from_str8(Str8 str, u32 radix) {
	s64 sign = sign_from_str8(str, &str);
	s64 x = (s64)u64_from_str8(str, radix) * sign;
	return x;
}

f64 f64_from_str8(Str8 str) {
	if (str.size == 0) return 0;
	f64 result = 0;
	str = str8_skip_chop_whitespace(str);
	s64 sign = sign_from_str8(str, &str);

	// detect infinity or NaN
	const u32 flags = StrMatchFlags_CaseInsensitive;
	if (str8_match(str8n(str.str, 3), str8("inf"), flags)) {
		union { u64 u; f64 f; } r;
		r.u = 0x7FF0000000000000ull;
		r.f *= sign;
		return r.f;
	} else if (str8_match(str8n(str.str, 3), str8("nan"), flags)) {
		union { u64 u; f64 f; } r;
		r.u = 0x7FFFFFFFFFFFFFFFull;
		r.f *= sign;
		return r.f;
	}

	// parse decimal
	for (u64 i = 0; i < str.size; i++) {
		if (!char_is_digit(str.str[i], 10)) {
			str = str8_skip(str, i);
			break;
		}
		result *= 10;
		result += integer_symbol_reverse[str.str[i] & 0x7F];
	}

	// parse fractional
	if (str.str[0] == '.') {
		f64 denom = 10.0;
		str = str8_skip(str, 1);
		for (u64 i = 0; i < str.size; i++) {
			if (!char_is_digit(str.str[i], 10)) {
				str = str8_skip(str, i);
				break;
			}
			result += (f64)(integer_symbol_reverse[str.str[i] & 0x7F]) / denom;
			denom *= 10;
		}
	}

	// parse exponent
	if (str.str[0] == 'e' || str.str[0] == 'E') {
		str = str8_skip(str, 1);
		b32 neg_exp = (sign_from_str8(str, &str) < 0);
		u64 exponent = u64_from_str8(str, 10);
		if (neg_exp) {
			for (u64 i = 1; i < exponent; i++) {
				result /= 10;
			}
		} else {
			for (u64 i = 1; i < exponent; i++) {
				result *= 10;
			}
		}
	}

	result *= sign;
	return result;
}

/* Unicode String decoding/conversion */

static u8 utf8_class[32] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

UnicodeDecode utf8_decode(u8 *str, u64 max) {
	UnicodeDecode result = {1, U32_MAX};
	u8 byte = str[0];
	u8 byte_class = utf8_class[byte >> 3];
	switch (byte_class) {
		case 1: {
			result.codepoint = byte;
		}break;
		case 2: {
			if (1 < max) {
				u8 cont_byte = str[1];
				if (utf8_class[cont_byte >> 3] == 0) {
					result.codepoint = (byte & ((1 << 5) - 1)) << 6;
					result.codepoint |=  (cont_byte & ((1 << 6) - 1));
					result.inc = 2;
				}
			}
		}break;
		case 3: {
			if (2 < max) {
				u8 cont_byte[2] = {str[1], str[2]};
				if (utf8_class[cont_byte[0] >> 3] == 0 &&
				    utf8_class[cont_byte[1] >> 3] == 0) {
					result.codepoint = (byte & ((1 << 4) - 1)) << 12;
					result.codepoint |= ((cont_byte[0] & ((1 << 6) - 1)) << 6);
					result.codepoint |=  (cont_byte[1] & ((1 << 6) - 1));
					result.inc = 3;
				}
			}
		}break;
		case 4: {
			if (3 < max) {
				u8 cont_byte[3] = {str[1], str[2], str[3]};
				if (utf8_class[cont_byte[0] >> 3] == 0 &&
				    utf8_class[cont_byte[1] >> 3] == 0 &&
				    utf8_class[cont_byte[2] >> 3] == 0) {
					result.codepoint = (byte & ((1 << 3) - 1)) << 18;
					result.codepoint |= ((cont_byte[0] & ((1 << 6) - 1)) << 12);
					result.codepoint |= ((cont_byte[1] & ((1 << 6) - 1)) <<  6);
					result.codepoint |=  (cont_byte[2] & ((1 << 6) - 1));
					result.inc = 4;
				}
			}
		}
	}
	return result;
}

UnicodeDecode utf16_decode(u16 *str, u64 max) {
	UnicodeDecode result = {1, U32_MAX};
	result.codepoint = str[0];
	result.inc = 1;
	if (max > 1 && 0xD800 <= str[0] && str[0] < 0xDC00 && 0xDC00 <= str[1] && str[1] < 0xE000){
		result.codepoint = ((str[0] - 0xD800) << 10) | ((str[1] - 0xDC00) + 0x10000);
		result.inc = 2;
	}
	return result;
}

u32 utf8_encode(u8 *str, u32 codepoint) {
	u32 inc = 0;
	if (codepoint <= 0x7F) {
		str[0] = (u8)codepoint;
		inc = 1;
	} else if (codepoint <= 0x7FF) {
		str[0] = (((1 << 2) - 1) << 6) | ((codepoint >> 6) & ((1 << 5) - 1));
		str[1] = (0x80) | (codepoint & ((1 << 6) - 1));
		inc = 2;
	} else if (codepoint <= 0xFFFF) {
		str[0] = (((1 << 3) - 1) << 5) | ((codepoint >> 12) & ((1 << 4) - 1));
		str[1] = (0x80) | ((codepoint >> 6) & ((1 << 6) - 1));
		str[2] = (0x80) | ( codepoint       & ((1 << 6) - 1));
		inc = 3;
	} else if (codepoint <= 0x10FFFF) {
		str[0] = (((1 << 4) - 1) << 4) | ((codepoint >> 18) & ((1 << 3) - 1));
		str[1] = (0x80) | ((codepoint >> 12) & ((1 << 6) - 1));
		str[2] = (0x80) | ((codepoint >>  6) & ((1 << 6) - 1));
		str[3] = (0x80) | ( codepoint        & ((1 << 6) - 1));
		inc = 4;
	} else {
		str[0] = '?';
		inc = 1;
	}
	return inc;
}

u32 utf16_encode(u16 *str, u32 codepoint) {
	u32 inc = 1;
	if (codepoint == 0) {
		str[0] = (u16)'?';
	} else if (codepoint < 0x10000) {
		str[0] = (u16)codepoint;
	} else {
		u32 v = codepoint - 0x10000;
		str[0] = safe_cast_u16(0xD800 + (v >> 10));
		str[1] = safe_cast_u16(0xDC00 + (v & ((1 << 10) - 1)));
		inc = 2;
	}
	return inc;
}

Str8 str8_from_16(Arena *arena, Str16 in) {
	Str8 result = {0};
	if (in.size) {
		u64 cap = in.size*3;
		u8 *str = push_array_no_zero(arena, u8, cap + 1);
		u16 *ptr = in.str;
		u16 *opl = ptr + in.size;
		u64 size = 0;
		UnicodeDecode consume;
		for (;ptr < opl; ptr += consume.inc) {
			consume = utf16_decode(ptr, opl - ptr);
			size += utf8_encode(str + size, consume.codepoint);
		}
		str[size] = 0;
		arena_pop(arena, (cap - size));
		result = str8n(str, size);
	}
	return result;
}

Str16 str16_from_8(Arena *arena, Str8 in) {
	Str16 result = {0};
	if (in.size) {
		u64 cap = in.size * 2;
		u16 *str = push_array_no_zero(arena, u16, cap + 1);
		u8 *ptr = in.str;
		u8 *opl = ptr + in.size;
		u64 size = 0;
		UnicodeDecode consume;
		for (;ptr < opl; ptr += consume.inc) {
			consume = utf8_decode(ptr, opl - ptr);
			size += utf16_encode(str + size, consume.codepoint);
		}
		str[size] = 0;
		arena_pop(arena, (cap - size)*2);
		result = str16n(str, size);
	}
	return result;
}

Str8List str8_list_from_argcv(Arena *arena, int argc, char **argv) {
	Str8List result = {0};
	for (int i = 0; i < argc; i += 1) {
		Str8 str = str8c((u8 *)argv[i]);
		str8_list_push(arena, &result, str);
	}
	return result;
}

u64 u64_hash_from_seed_str8(u64 seed, Str8 str) {
	return XXH3_64bits_withSeed(str.str, str.size, seed);
}

u64 u64_hash_from_str8(Str8 str) {
	return XXH3_64bits(str.str, str.size);
}

/* Command line parsing */

CmdLineOpt **cmd_line_slot_from_string(CmdLine *cmd_line, Str8 str) {
	CmdLineOpt **slot = 0;
	if (cmd_line->opt_table_size != 0) {
		u64 hash = u64_hash_from_str8(str);
		u64 bucket = hash % cmd_line->opt_table_size;
		slot = &cmd_line->opt_table[bucket];
	}
	return slot;
}

CmdLineOpt *cmd_line_opt_from_slot(CmdLineOpt **slot, Str8 str) {
	CmdLineOpt *result = 0;
	for (CmdLineOpt *var = *slot; var; var = var->hash_next) {
		if (str8_match(str, var->str, 0)) {
			result = var;
			break;
		}
	}
	return result;
}

void cmd_line_push_opt(CmdLineOptList *list, CmdLineOpt *var) {
	if (!list->first) {
		list->first = list->last = var;
		var->next = NULL;
	} else {
		list->last->next = var;
		list->last = var;
		var->next = NULL;
	}
	list->count++;
}

CmdLineOpt *cmd_line_insert_opt(Arena *arena, CmdLine *cmd_line, Str8 str, Str8List values) {
	CmdLineOpt *var = 0;
	CmdLineOpt **slot = cmd_line_slot_from_string(cmd_line, str);
	CmdLineOpt *existing_var = cmd_line_opt_from_slot(slot, str);
	if (existing_var != 0) {
		var = existing_var;
	} else {
		var = push_array(arena, CmdLineOpt, 1);
		var->hash_next = *slot;
		var->hash = u64_hash_from_str8(str);
		var->str = str8_copy(arena, str);
		var->value_strs = values;
		StrJoin join = {0};
		join.pre = str8("");
		join.sep = str8(",");
		join.post = str8("");
		var->value_str = str8_list_join(arena, &var->value_strs, &join);
		*slot = var;
		cmd_line_push_opt(&cmd_line->opts, var);
	}
	return var;
}

CmdLine cmd_line_from_string_list(Arena *arena, Str8List list) {
	CmdLine parsed = {0};
	parsed.exe_name = list.first->str;
	parsed.opt_table_size = 64;
	parsed.opt_table = push_array(arena, CmdLineOpt *, parsed.opt_table_size);

	// parse options / inputs
	b8 after_passthrough_option = 0;
	b8 first_passthrough = 1;
	for (Str8Node *node = list.first->next, *next = 0; node != 0; node = next) {
		next = node->next;

		// Look at `--`, or `-` at the start of an argument to determine if it's a flag option.
		// All arguments after a single "--" (with no trailing string on the command line will be
		// considered as passthrough input strings.)
		b8 is_option = 0;
		Str8 opt_name = node->str;
		if (!after_passthrough_option) {
			is_option = 1;
			if (str8_match(node->str, str8("--"), 0)) {
				after_passthrough_option = 1;
				is_option = 0;
			} else if (str8_match(str8_prefix(node->str, 2), str8("--"), 0)) {
				opt_name = str8_skip(opt_name, 2);
			} else if (str8_match(str8_prefix(node->str, 1), str8("-"), 0)) {
				opt_name = str8_skip(opt_name, 1);
			} else if (str8_match(str8_prefix(node->str, 1), str8("/"), 0)) {
				opt_name = str8_skip(opt_name, 1);
			} else {
				is_option = 0;
			}
		}

		// string is an option
		if (is_option) {
			// unpack option prefix
			b8 has_values = 0;
			u64 value_signifier_position1 = str8_find_needle(opt_name, 0, str8(":"), 0);
			u64 value_signifier_position2 = str8_find_needle(opt_name, 0, str8("="), 0);
			u64 value_signifier_position = Min(value_signifier_position1, value_signifier_position2);
			Str8 value_portion_this_str = str8_skip(opt_name, value_signifier_position+1);
			if (value_signifier_position < opt_name.size) {
				has_values = 1;
			}
			opt_name = str8_prefix(opt_name, value_signifier_position);

			// parse option's values
			Str8List values = {0};
			if (has_values) {
				for (Str8Node *n = node; n; n = n->next) {
					next = n->next;
					Str8 str = n->str;
					if (n == node) {
						str = value_portion_this_str;
					}
					Str8 splits = str8(",");
					Str8List values_in_this_str = str8_split(arena, str, splits);
					for (Str8Node *sub_val = values_in_this_str.first; sub_val; sub_val = sub_val->next) {
						str8_list_push(arena, &values, sub_val->str);
					}
					if (!str8_match(str8_postfix(n->str, 1), str8(","), 0) &&
					    (n != node || value_portion_this_str.size != 0)) {
						break;
					}
				}
			}

			// store
			cmd_line_insert_opt(arena, &parsed, opt_name, values);
		} else if (!str8_match(node->str, str8("--"), 0) || !first_passthrough) {
			// default path - treat as a passthrough input
			str8_list_push(arena, &parsed.inputs, node->str);
			first_passthrough = 0;
		}
	}

	return parsed;
}

CmdLine cmdline_from_argcv(Arena *arena, int argc, char **argv) {
	CmdLine result;
	result.raw_args = str8_list_from_argcv(arena, argc, argv);
	result = cmd_line_from_string_list(arena, result.raw_args);
	result.argc = argc;
	result.argv = argv;
	return result;
}

/* Printing */

void m_printf(char *fmt, ...) {
	Temp scratch = scratch_begin(NULL, 0);
	va_list args;
	va_start(args, fmt);
	Str8 out = str8fv(scratch.arena, fmt, args);
	write_string_to_stdout(out);
	scratch_end(scratch);
}

void m_eprintf(char *fmt, ...) {
	Temp scratch = scratch_begin(NULL, 0);
	va_list args;
	va_start(args, fmt);
	Str8 out = str8fv(scratch.arena, fmt, args);
	write_string_to_stderr(out);
	scratch_end(scratch);
}

/* OS specific stuff */

#if defined(_WIN32)
#include <Windows.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

struct {
	u64 ms_resolution;
	u64 page_size;
	b8 initialized;
} w32_state;

void w32_init_state(void) {
	SYSTEM_INFO sysinfo = {0};
	GetSystemInfo(&sysinfo);
	w32_state.page_size = sysinfo.dwPageSize;
	LARGE_INTEGER resolution;
	QueryPerformanceFrequency(&resolution);
	w32_state.ms_resolution = resolution.QuadPart;
	w32_state.initialized = 1;
}

u64 get_page_size(void) {
	if (!w32_state.initialized) w32_init_state();
	return w32_state.page_size;
}

void *mem_reserve(u64 size) {
	return VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
}

b8 mem_commit(void *ptr, u64 size) {
	return (VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != 0);
}

void mem_decommit(void *ptr, u64 size) {
	VirtualFree(ptr, size, MEM_DECOMMIT);
}

void mem_release(void *ptr, u64 size) {
	// NOTE: size not used - not necessary on Windows, but necessary for other OSes.
	(void)size;
	VirtualFree(ptr, 0, MEM_RELEASE);
}

/* File I/O */

Str8 data_from_file(Arena *arena, Str8 path) {
	Temp scratch = scratch_begin(NULL, 0);
	Str16 path16 = str16_from_8(scratch.arena, path);
	HANDLE file = CreateFileW((WCHAR *)path16.str, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	Str8 result = {0};
	if (file != INVALID_HANDLE_VALUE) {
		BY_HANDLE_FILE_INFORMATION info;
		GetFileInformationByHandle(file, &info);
		u64 size = (u64)info.nFileSizeLow | (((u64)info.nFileSizeHigh)<<32);
		result.size = size;
		result.str = push_array_no_zero(arena, u8, result.size);
		u64 bytes_read = 0;
		while (bytes_read < size) {
			u64 amt64 = size - bytes_read;
			u32 amt32 = (amt64 > U32_MAX) ? U32_MAX : (u32)amt64;
			DWORD read_size = 0;
			ReadFile(file, result.str + bytes_read, amt32, &read_size, NULL);
			bytes_read += read_size;
			if (read_size != amt32) break;
		}
		CloseHandle(file);
	}
	scratch_end(scratch);
	return result;
}

b8 write_to_file(Str8 data, Str8 path) {
	b8 good = 1;
	Temp scratch = scratch_begin(NULL, 0);
	Str16 path16 = str16_from_8(scratch.arena, path);
	HANDLE file = CreateFileW((WCHAR *)path16.str, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (file != INVALID_HANDLE_VALUE) {
		u64 total = 0;
		while (total < data.size) {
			u64 amt64 = data.size - total;
			u32 amt32 = (amt64 > U32_MAX) ? U32_MAX : (u32)amt64;
			DWORD bytes_written = 0;
			BOOL success = WriteFile(file, data.str + total, amt32, &bytes_written, NULL);
			total += bytes_written;
			if (!success) {
				good = 0;
				break;
			}
		}
		CloseHandle(file);
	}
	scratch_end(scratch);
	return good;
}

/* Time */

u64 now_us(void) {
	if (!w32_state.initialized) w32_init_state();
	u64 result = 0;
	LARGE_INTEGER large_int_counter;
	QueryPerformanceCounter(&large_int_counter);
	result = (large_int_counter.QuadPart * 1000000) / w32_state.ms_resolution;
	return result;
}

void sleep_ms(u32 ms) {
	Sleep(ms);
}

/* NOTE: Entrypoint */
#ifndef MAIN_HANDLED

raddbg_entry_point(entry_point);

void w32_write_to_stdfile(Str8 str, DWORD stdfile) {
	HANDLE file = GetStdHandle(stdfile);
	u64 total = 0;
	while (total < str.size) {
		u64 amt64 = str.size - total;
		u32 amt32 = (amt64 > U32_MAX) ? U32_MAX : (u32)amt64;
		DWORD bytes_written = 0;
		WriteFile(file, str.str + total, amt32, &bytes_written, NULL);
		total += bytes_written;
	}
}

void write_string_to_stdout(Str8 str) {
	w32_write_to_stdfile(str, STD_OUTPUT_HANDLE);
}

void write_string_to_stderr(Str8 str) {
	w32_write_to_stdfile(str, STD_ERROR_HANDLE);
}

#include <vcruntime_startup.h>
#ifdef BUILD_CONSOLE
int APIENTRY mainCRTStartup(void)
#else
int APIENTRY WinMainCRTStartup(void)
//int APIENTRY WinMain(HINSTANCE d, HINSTANCE p, LPSTR a, int s)
// (void)d; (void)p; (void)a; (void)s;
#endif
{
	__isa_available_init();
#ifdef BUILD_CONSOLE
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	w32_init_state();

	Arena *args_arena = arena_alloc(.reserve_size = MB(1), .commit_size = KB(32));
	int argc;
	WCHAR **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char **argv = push_array(args_arena, char *, argc);
	for (int i = 0; i < argc; i++) {
		Str8 arg = str8_from_16(args_arena, str16c((u16 *)wargv[i]));
		argv[i] = (char *)arg.str;
	}
	CmdLine cmd = cmdline_from_argcv(args_arena, argc, argv);

	int exit_code = entry_point(&cmd);

	LocalFree(wargv);
	arena_release(args_arena);
	ExitProcess(exit_code);
}
#endif

/* END _WIN32 */
#else
#error OS specific functionality not implemented for this OS!
#endif
