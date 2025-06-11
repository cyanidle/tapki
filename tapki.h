#pragma once
#ifndef TAPKI_H
#define TAPKI_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

//#undef TAPKI_CLI_NO_TTY

#ifdef __GNUC__
    #define TAPKI_NORETURN __attribute__((noreturn))
    #define TAPKI_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define TAPKI_ALLOC_ATTR(sz, al) __attribute__((malloc, alloc_size(sz), alloc_align(al)))
    #define TAPKI_FMT_ATTR(fmt, args) __attribute__((format(printf, fmt, args)))
#else
    #if defined(_MSC_VER)
        #define TAPKI_NORETURN __declspec(noreturn)
    #else
        #define TAPKI_NORETURN
    #endif
    #define TAPKI_UNLIKELY(x) x
    #define TAPKI_ALLOC_ATTR(sz, al)
    #define TAPKI_FMT_ATTR(fmt, args)
#endif

// If TAPKI_FULL_NAMESPACE is not defined -> you can use Public API without Tapki prefix

TAPKI_FMT_ATTR(1, 2) TAPKI_NORETURN
void TapkiDie(const char* __restrict__ fmt, ...);
#define TapkiAssert(...) TapkiFrame() { if (!(__VA_ARGS__)) Die("Assertion failed: " #__VA_ARGS__); }

// --- Arena
typedef struct TapkiArena TapkiArena;

TapkiArena* TapkiArenaCreate(size_t chunkSize);
TAPKI_ALLOC_ATTR(2, 3) void* TapkiArenaAllocAligned(TapkiArena* arena, size_t size, size_t align);
void* TapkiArenaAlloc(TapkiArena* arena, size_t size);
char* TapkiArenaAllocChars(TapkiArena* arena, size_t count);
void TapkiArenaClear(TapkiArena* arena);
void TapkiArenaFree(TapkiArena* arena);
// ---

// --- Vectors
#define TapkiVec(type) struct { type* d; size_t size; size_t cap; }
#define TapkiVecShrink(arena, vec) __tapki_vec_shrink(arena, (vec), TapkiVecS(vec))
#define TapkiVecT(vec) __typeof__(*(vec)->d)
#define TapkiVecS(vec) sizeof(*(vec)->d)
#define TapkiVecA(vec) _Alignof(TapkiVecT(vec))
#define TapkiVecSA(vec) TapkiVecS(vec), TapkiVecA(vec)
#define TapkiVecAppend(arena, vec, ...) __tapki_vec_append((arena), (vec), __TapkiArr(TapkiVecT(vec), __VA_ARGS__), TapkiVecSA(vec))
#define TapkiVecPush(arena, vec) ((TapkiVecT(vec)*)__tapki_vec_push((arena), (vec), TapkiVecSA(vec)))
#define TapkiVecPop(vec)  ((TapkiVecT(vec)*)__tapki_vec_pop((vec), TapkiVecS(vec)))
#define TapkiVecAt(vec, idx)  ((TapkiVecT(vec)*)__tapki_vec_at((vec), idx, TapkiVecS(vec)) + idx)
#define TapkiVecErase(vec, idx)  __tapki_vec_erase((vec), idx, TapkiVecS(vec))
#define TapkiVecInsert(arena, vec, idx)  ((TapkiVecT(vec)*)__tapki_vec_insert((arena), (vec), idx, TapkiVecSA(vec)))
#define TapkiVecForEach(vec, it) for (TapkiVecT(vec)* it = (vec)->d; it && it != (vec)->d + (vec)->size; ++it)
#define TapkiVecForEachRev(vec, it) for (TapkiVecT(vec)* it = (vec)->d + (vec)->size - 1; (vec)->d && it != (vec)->d - 1; --it)
void    TapkiVecClear(void* _vec);
#define TapkiVecReserve(arena, vec, n) __tapki_vec_reserve((arena), (vec), n, TapkiVecSA(vec))
#define TapkiVecResize(arena, vec, n) __tapki_vec_resize((arena), (vec), n, TapkiVecSA(vec))

typedef TapkiVec(char) TapkiStr;
typedef TapkiVec(TapkiStr) TapkiStrVec;
typedef TapkiVec(int64_t) TapkiIntVec;
// ---

// --- Maps
#define TapkiMapDeclare(Name, K, V) \
    typedef struct{ const K key; V value; } Name##_Pair; \
    typedef TapkiVec(Name##_Pair) Name; \
    typedef const K Name##_Key; \
    typedef V Name##_Value; \
    Name##_Value* Name##_Find(const Name* map, Name##_Key key); \
    Name##_Value* Name##_At(TapkiArena* arena, Name* map, Name##_Key key); \
    bool Name##_Erase(Name* map, Name##_Key key) \

#define TapkiMapKT(map) __typeof__((map)->d->key)
#define TapkiMapVT(map) __typeof__((map)->d->value)

TapkiMapDeclare(TapkiStrMap, char*, TapkiStr);

#define TapkiMapImplement(Name, Less, Eq) TapkiMapImplement1(Name, Less, Eq)

#define TAPKI_TRIVIAL_LESS(l, r) ((l) < (r))
#define TAPKI_TRIVIAL_EQ(l, r) ((l) == (r))

#define TAPKI_STRING_LESS(l, r) (strcmp((l), (r)) < 0)
#define TAPKI_STRING_EQ(l, r) (strcmp((l), (r)) == 0)
// ---

// --- Strings
#define Tapki_npos ((size_t)-1)
#define TapkiStrAppend(arena, s, ...) __tapkis_append((arena), (s), __TapkiArr(const char*, __VA_ARGS__))

TapkiStr TapkiStrSub(TapkiArena *ar, const char* target, size_t from, size_t to);
TAPKI_FMT_ATTR(3, 4) TapkiStr* TapkiStrAppendF(TapkiArena *ar, TapkiStr* str, const char* __restrict__ fmt, ...);
TapkiStr* TapkiStrAppendVF(TapkiArena *ar, TapkiStr* str, const char* __restrict__ fmt, va_list list);
size_t TapkiStrFind(const char* target, const char* what, size_t offset);
TapkiStr TapkiStrCopy(TapkiArena *ar, const char* target, size_t len);
size_t TapkiStrRevFind(const char* target, const char* what);
bool TapkiStrContains(const char* target, const char* what);
bool TapkiStrStartsWith(const char* target, const char* what);
bool TapkiStrEndsWith(const char* target, const char* what);
TapkiStrVec TapkiStrSplit(TapkiArena *ar, const char* target, const char *delim);
TAPKI_FMT_ATTR(2, 3) TapkiStr TapkiF(TapkiArena* ar, const char* __restrict__ fmt, ...);
TapkiStr TapkiVF(TapkiArena* ar, const char* __restrict__ fmt, va_list list);
TapkiStr TapkiS(TapkiArena* ar, const char* s);

int32_t TapkiToI32(const char* s);
uint32_t TapkiToU32(const char* s);
int64_t TapkiToI64(const char* s);
uint64_t TapkiToU64(const char* s);
double TapkiToFloat(const char* s);
// ---

// --- Files
TapkiStr TapkiReadFile(TapkiArena* ar, const char* file);
void TapkiWriteFile(const char* file, const char* contents);
void TapkiAppendFile(const char* file, const char* contents);
#define TapkiPathJoin(arena, ...) __tpk_path_join(arena, __TapkiArr(const char*, __VA_ARGS__))
// ---

// --- Tracebacks
#define TapkiFrameF(fmt, ...) for( \
    __tpk_scope __scope = (__tpk_scope){__FILE__":"__TPK_STR(__LINE__), __func__}; \
    !__scope.__f && (fmt ? snprintf(__scope.msg, sizeof(__scope.msg), fmt ? fmt : "!", ##__VA_ARGS__) : (void)0, __tpk_frame_start(&__scope), 1); \
    __tpk_frame_end(), __scope.__f = 1 \
)
#define TapkiFrame() TapkiFrameF(NULL)
#define TapkiFramesIter(frame) TapkiVecForEachRev(&__tpk_gframes.frames, frame)
TapkiStr TapkiTraceback(TapkiArena* arena);
// ---


// --- CLI
typedef struct TapkiCLIVarsResult {
    bool ok;
    bool need_help;
    TapkiStr error;
} TapkiCLIVarsResult;

typedef struct TapkiCLI {
    const char* name; // pos_arg_name,pos_arg_name2 / --named,-n,--named2
    void* data; // pointer to variable of fitting type
    // all other are optional:
    const char* help;
    const char* metavar;
    bool flag; // kw-only: Treat as a switch
    bool int64; // Parse as int64, not Str
    bool required;
    bool many; // Parse into Vector of values
    bool program; // this option is for the whole program (names -> prog name, help -> description)
} TapkiCLI;

// Example:
// Str name = {0}; bool flag = {0};
// CLI cli[] = { {"-n,--name", &name}, {"-s,--switch", &flag, .is_flag=true}, {0} };
// int ret = ParseCLI(arena, cli, argc, argv);
// if (ret != 0) return ret;

int TapkiParseCLI(TapkiArena *ar, TapkiCLI cli[], int argc, char **argv);

// Lower level CLI api
TapkiCLIVarsResult TapkiCLI_ParseVars(TapkiArena *ar, const TapkiCLI cli[], int argc, char **argv);
TapkiStr TapkiCLI_Usage(TapkiArena *ar, const TapkiCLI cli[], int argc, char **argv);
TapkiStr TapkiCLI_Help(TapkiArena *ar, const TapkiCLI cli[]);
// ---


// Short API
#ifndef TAPKI_FULL_NAMESPACE

typedef TapkiArena Arena;
typedef TapkiStr Str;
typedef TapkiStrMap StrMap;
typedef TapkiStrVec StrVec;
typedef TapkiIntVec IntVec;
typedef TapkiCLI CLI;

#define Vec(type)                       TapkiVec(type)
#define VecShrink(vec)                  TapkiVecShrink(arena, vec)
#define VecAppend(vec, ...)             TapkiVecAppend(arena, vec, ##__VA_ARGS__)
#define VecPush(vec)                    TapkiVecPush(arena, vec)
#define VecPop(vec)                     TapkiVecPop(vec)
#define VecAt(vec, idx)                 TapkiVecAt(vec, idx)
#define VecForEach(vec, it)             TapkiVecForEach(vec, it)
#define VecForEachRev(vec, it)          TapkiVecForEachRev(vec, it)
#define VecErase(vec, idx)              TapkiVecErase(vec, idx)
#define VecInsert(vec, idx)             TapkiVecInsert(arena, vec, idx)
#define VecClear(vec)                   TapkiVecClear(vec)
#define VecReserve(vec, n)              TapkiVecReserve(arena, vec, n)
#define VecResize(vec, n)               TapkiVecResize(arena, vec, n)

#define ArenaCreate(chunksize)          TapkiArenaCreate(chunksize)
#define ArenaAllocAligned(ar, sz, al)   TapkiArenaAllocAligned(ar, sz, al)
#define ArenaAlloc(arena, sz)           TapkiArenaAlloc(arena, sz)
#define ArenaClear(arena)               TapkiArenaClear(arena)
#define ArenaFree(arena)                TapkiArenaFree(arena)

#define F(fmt, ...)                     TapkiF(arena, fmt, ##__VA_ARGS__)
#define S(str)                          TapkiS(arena, str)

#define ToI32(str)                      TapkiToI32(str)
#define ToU32(str)                      TapkiToU32(str)
#define ToI64(str)                      TapkiToI64(str)
#define ToU64(str)                      TapkiToU64(str)
#define ToFloat(str)                    TapkiToFloat(str)

#define StrAppend(s, part, ...)         TapkiStrAppend(arena, s, part, ##__VA_ARGS__)
#define StrAppendF(s, fmt, ...)         TapkiStrAppendF(arena, s, fmt, ##__VA_ARGS__)
#define StrSplit(s, delim)              TapkiStrSplit(arena, s, delim)
#define StrSub(s, from, to)             TapkiStrSub(arena, s, from, to)
#define StrFind(s, needle, offs)        TapkiStrFind(s, needle, offs)
#define StrCopy(chars, len)             TapkiStrCopy(arena, chars, len)
#define StrRevFind(s, needle)           TapkiStrRevFind(s, needle)
#define StrContains(s, needle)          TapkiStrContains(s, needle)
#define StrStartsWith(s, needle)        TapkiStrStartsWith(s, needle)
#define StrEndsWith(s, needle)          TapkiStrEndsWith(s, needle)
#define npos                            Tapki_npos

#define StrMap_At(map, key)             TapkiStrMap_At(arena, map, key)
#define StrMap_Find(map, key)           TapkiStrMap_Find(map, key)
#define StrMap_Erase(map, key)          TapkiStrMap_Erase(map, key)

#define MapDeclare(map, key, value)     TapkiMapDeclare(map, key, value)
#define MapImplement(map, less, eq)     TapkiMapImplement(map, less, eq)
#define TRIVIAL_LESS                    TAPKI_TRIVIAL_LESS
#define TRIVIAL_EQ                      TAPKI_TRIVIAL_EQ
#define STRING_LESS                     TAPKI_STRING_LESS
#define STRING_EQ                       TAPKI_STRING_EQ

#define Die(fmt, ...)                   TapkiDie(fmt, ##__VA_ARGS__)
#define Assert(...)                     TapkiAssert(...)

#define WriteFile(file, data)           TapkiWriteFile(file, data)
#define AppendFile(file, data)          TapkiAppendFile(file, data)
#define ReadFile(file)                  TapkiReadFile(arena, file)

#define PathJoin(...)                   TapkiPathJoin(arena, __VA_ARGS__)

#define ParseCLI(cli, argc, argv)       TapkiParseCLI(arena, cli, argc, argv)

#define FrameF(fmt, ...)                TapkiFrameF(fmt, ##__VA_ARGS__)
#define Frame()                         TapkiFrame()
#define Traceback()                     TapkiTraceback(arena)

#endif

// Short API END











// --- Private stuff


#define TapkiMapImplement1(Name, Less, Eq) \
static bool __##Name##_eq(const void* _lhs, const void* _rhs) { \
    Name##_Key lhs = *(Name##_Key*)_lhs; \
    Name##_Key rhs = *(Name##_Key*)_rhs; \
    return Eq(lhs, rhs); \
} \
static void* __##Name##_lower_bound(const void* _begin, const void* _end, const void* _key) { \
    Name##_Pair* begin = (Name##_Pair*)_begin; \
    Name##_Pair* end = (Name##_Pair*)_end; \
    Name##_Pair* it;  \
    Name##_Key key = *(Name##_Key*)_key; \
    size_t count, step;  \
    count = end - begin;  \
    while (count > 0) {  \
        it = begin;  \
        step = count / 2; \
        it += step; \
        if (Less(it->key, key)) { \
            begin = ++it; \
            count -= step + 1; \
        } else { \
                count = step; \
        } \
    } \
    return begin; \
} \
static const __tpk_map_info __##Name##_info = { \
    __##Name##_eq, __##Name##_lower_bound, \
    sizeof(Name##_Key), sizeof(Name##_Pair), \
    _Alignof(Name##_Pair), offsetof(Name##_Pair, value) \
}; \
Name##_Value *Name##_At(TapkiArena *ar, Name *map, Name##_Key key) {  \
    return (Name##_Value*)__tapki_map_at(ar, map, &key, &__##Name##_info);  \
} \
Name##_Value *Name##_Find(const Name *map, Name##_Key key) {  \
    return (Name##_Value*)__tapki_map_find(map, &key, &__##Name##_info);  \
} \
bool Name##_Erase(Name *map, Name##_Key key) { \
    return __tapki_map_erase(map, &key, &__##Name##_info); \
} bool Name##_Erase(Name *map, Name##_Key key)

#define __TapkiArr(t, ...) (t[]){__VA_ARGS__}, PP_NARG(__VA_ARGS__)

typedef struct {
    char* d;
    size_t size;
    size_t cap;
} __TapkiVec;

TapkiStr __tpk_path_join(TapkiArena* ar, const char** parts, size_t count);
void* __tapki_vec_insert(TapkiArena* ar, void* _vec, size_t idx, size_t tsz, size_t al);
char* __tapki_vec_reserve(TapkiArena* ar, void* _vec, size_t count, size_t tsz, size_t al);
char* __tapki_vec_resize(TapkiArena* ar, void* _vec, size_t count, size_t tsz, size_t al);
bool __tapki_vec_shrink(TapkiArena* ar, void* _vec, size_t tsz);
void __tapki_vec_append(TapkiArena* ar, void* _vec, void* data, size_t count, size_t tsz, size_t al);
TapkiStr* __tapkis_append(TapkiArena *ar, TapkiStr* target, const char **src, size_t count);
void __tapki_vec_erase(void* _vec, size_t idx, size_t tsz);

typedef struct {
    bool(*eq)(const void* lhs, const void* rhs);
    void*(*lower_bound)(const void* beg, const void* end, const void* key);
    int key_sizeof;
    int pair_sizeof;
    int pair_align;
    int value_offset;
} __tpk_map_info;

void* __tapki_map_at(TapkiArena *ar, void* _map, const void* key, const __tpk_map_info* info);
void* __tapki_map_find(const void* _map, const void* key, const __tpk_map_info* info);
bool __tapki_map_erase(void *map, const void *key, const __tpk_map_info* info);

#define __TPK_STR2(x) #x
#define __TPK_STR(x) __TPK_STR2(x)

typedef struct {
    const char* loc;
    const char* func;
    char msg[124];
    int __f;
} __tpk_scope;

void __tpk_frame_start(__tpk_scope* scope);
void __tpk_frame_vstart(__tpk_scope* scope, ...);
void __tpk_frame_end();

typedef struct __tpk_frame {
    __tpk_scope* s;
} __tpk_frame;

typedef struct __tpk_frames {
    TapkiVec(__tpk_frame) frames;
    TapkiArena* arena;
} __tpk_frames;

extern _Thread_local __tpk_frames __tpk_gframes;


static inline void* __tapki_vec_push(TapkiArena* ar, void* _vec, size_t tsz, size_t al) {
    __TapkiVec* vec = (__TapkiVec*)_vec;
    return (vec->size >= vec->cap ? __tapki_vec_reserve(ar, vec, vec->cap + 1, tsz, al) : vec->d) + (vec->size++ * tsz);
}

static inline void* __tapki_vec_pop(void* _vec, size_t tsz) {
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (!vec->size) TapkiDie("vector.pop: Vector is empty");
    return vec->d + (vec->size-- * tsz);
}

static inline void* __tapki_vec_at(void* _vec, size_t pos, size_t tsz) {
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (vec->size <= pos) TapkiDie("vector.at: index(%zu) > size(%zu)", pos, vec->size);
    return vec->d;
}

#define PP_NARG(...) \
    PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) \
    PP_128TH_ARG(__VA_ARGS__)
#define PP_128TH_ARG(                                           \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,                    \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,           \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30,           \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40,           \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,           \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60,           \
    _61, _62, _63, _64, _65, _66, _67, _68, _69, _70,           \
    _71, _72, _73, _74, _75, _76, _77, _78, _79, _80,           \
    _81, _82, _83, _84, _85, _86, _87, _88, _89, _90,           \
    _91, _92, _93, _94, _95, _96, _97, _98, _99, _100,          \
    _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, \
    _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, \
    _121, _122, _123, _124, _125, _126, _127, N, ...) N
#define PP_RSEQ_N()                                       \
    127, 126, 125, 124, 123, 122, 121, 120,               \
    119, 118, 117, 116, 115, 114, 113, 112, 111, 110, \
    109, 108, 107, 106, 105, 104, 103, 102, 101, 100, \
    99, 98, 97, 96, 95, 94, 93, 92, 91, 90,           \
    89, 88, 87, 86, 85, 84, 83, 82, 81, 80,           \
    79, 78, 77, 76, 75, 74, 73, 72, 71, 70,           \
    69, 68, 67, 66, 65, 64, 63, 62, 61, 60,           \
    59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           \
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           \
    39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           \
    29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           \
    19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           \
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#ifdef __cplusplus
}
#endif

#endif //TAPKI_H





#ifdef TAPKI_IMPLEMENTATION


#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _WIN32
static const char* __tpk_sep = "\\"
#else
static const char* __tpk_sep = "/";
#endif

#define _TAPKI_MEMCPY(dest, src, count) if (src && count) memcpy(dest, src, count)

_Thread_local __tpk_frames __tpk_gframes;

void __tpk_frame_start(__tpk_scope *scope)
{
    struct __tpk_frames* frames = &__tpk_gframes;
    if (TAPKI_UNLIKELY(!frames->arena)) {
        frames->arena = TapkiArenaCreate(1024);
    }
    *TapkiVecPush(frames->arena, &frames->frames) = (__tpk_frame){scope};
}

void __tpk_frame_end()
{
    struct __tpk_frames* frames = &__tpk_gframes;
    __tpk_scope *scope = TapkiVecPop(&frames->frames)->s;
}

void __tapki_vec_erase(void *_vec, size_t idx, size_t tsz)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (TAPKI_UNLIKELY(vec->size <= idx))
        TapkiDie("vector.erase: index(%zu) > size(%zu)", idx, vec->size);
    size_t tail = (--vec->size) - idx;
    if (tail) {
        char* dest = vec->d + idx * tsz;
        char* src = dest + tsz;
        memmove(dest, src, tail * tsz);
    }
    if (tsz == 1 && vec->d)
        vec->d[vec->size] = 0;
}

TapkiStr TapkiTraceback(TapkiArena *arena)
{
    TapkiStr result = TapkiS(arena, "Traceback (most recent on top):\n");
    int longest = 0;
    TapkiFramesIter(frame) {
        int len = strlen(frame->s->loc);
        if (len > longest) longest = len;
    }
    TapkiFramesIter(frame) {
        TapkiStr msg = TapkiF(arena, "  %-*s in '%s()'", longest, frame->s->loc, frame->s->func);
        TapkiStrAppend(arena, &result, msg.d);
        if (frame->s->msg[0]) {
            TapkiStrAppend(arena, &result, " => ");
            TapkiStrAppend(arena, &result, frame->s->msg);
        }
        TapkiVecAppend(arena, &result, '\n');
    }
    return longest ? result : (TapkiStr){0};
}

void TapkiDie(const char *fmt, ...)
{
    static bool _recursive = false;
    if (_recursive) {
        fputs("Recursive Die()! Out of memory?\n", stderr);
        fflush(stderr);
        exit(1);
    }
    _recursive = true;
    fputs("Fatal Error: ", stderr);
    { //user msg
        va_list vargs;
        va_start(vargs, fmt);
        vfprintf(stderr, fmt, vargs);
        va_end(vargs);
        fputc('\n', stderr);
    }
    TapkiArena* arena = TapkiArenaCreate(1024);
    const char* trace = TapkiTraceback(arena).d;
    if (trace) {
        fputs(trace, stderr);
        fputc('\n', stderr);
    }
    fflush(stderr);
    exit(1);
}

void TapkiVecClear(void *_vec)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    vec->size = 0;
    if (vec->d) vec->d[0] = 0;
}

static TapkiStr __tapkis_withn(TapkiArena* ar, size_t res) {
    TapkiStr str = {0};
    TapkiVecReserve(ar, &str, res);
    str.size = res;
    str.d[res] = 0;
    return str;
}

void __tapki_vec_append(TapkiArena *ar, void *_vec, void *data, size_t count, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    __tapki_vec_reserve(ar, vec, vec->size + count, tsz, al);
    _TAPKI_MEMCPY(vec->d + vec->size * tsz, data, count * tsz);
    vec->size += count;
    if (tsz == 1 && vec->d)
        vec->d[vec->size] = 0;
}

TapkiStr* __tapkis_append(TapkiArena *ar, TapkiStr *target, const char** strs, size_t count)
{
    size_t total = 0;
    size_t lens[count];
    for (size_t i = 0; i < count; ++i) {
        total += (lens[i] = strlen(strs[i]));
    }
    TapkiVecReserve(ar, target, target->size + total + 1);
    for (size_t i = 0; i < count; ++i) {
        void* out = target->d + target->size;
        _TAPKI_MEMCPY(out, strs[i], lens[i]);
        target->size += lens[i];
    }
    if (target->d) {
        target->d[target->size] = 0;
    }
    return target;
}

TapkiStr TapkiStrSub(TapkiArena *ar, const char *target, size_t from, size_t to)
{
    assert(to >= from);
    if (!target) target = "";
    size_t tsz = strlen(target);
    assert(from <= tsz);
    to = to > tsz ? tsz : to;
    TapkiStr res = {0};
    size_t diff = (size_t)(to - from);
    TapkiVecReserve(ar, &res, diff);
    _TAPKI_MEMCPY(res.d, target + from, diff);
    return res;
}

/* By liw. */
static const char *__rev_strstr(const char *haystack, const char *needle) {
    if (*needle == '\0')
        return (char *) haystack;
    const char *result = NULL;
    for (;;) {
        const char *p = strstr(haystack, needle);
        if (p == NULL)
            break;
        result = p;
        haystack = p + 1;
    }
    return result;
}

TapkiStr TapkiStrCopy(TapkiArena *ar, const char* target, size_t len)
{
    TapkiStr res = __tapkis_withn(ar, len);
    _TAPKI_MEMCPY(res.d, target, len);
    return res;
}

size_t TapkiStrFind(const char *target, const char *what, size_t offset)
{
    if (!target) target = "";
    if (!what) what = "";
    const char* found = strstr(target + offset, what);
    if (!found) return Tapki_npos;
    return (size_t)(found - target);
}

size_t TapkiStrRevFind(const char *target, const char *what)
{
    if (!target) target = "";
    if (!what) what = "";
    const char* found = __rev_strstr(target, what);
    return found ? (size_t)(found - target) : Tapki_npos;
}

bool TapkiStrContains(const char *target, const char *what)
{
    if (!target) target = "";
    if (!what) what = "";
    return TapkiStrFind(target, what, 0) != Tapki_npos;
}

bool TapkiStrStartsWith(const char *target, const char *what)
{
    if (!target) target = "";
    if (!what) what = "";
    return TapkiStrFind(target, what, 0) == 0;
}

bool TapkiStrEndsWith(const char *target, const char *what)
{
    if (!target) target = "";
    if (!what) what = "";
    size_t tlen = strlen(target);
    size_t wlen = strlen(what);
    if (wlen > tlen) return false;
    return TapkiStrFind(target, what, tlen - wlen);
}

TapkiStrVec TapkiStrSplit(TapkiArena *ar, const char *target, const char *delim)
{
    if (!target) target = "";
    if (!delim) delim = "";
    TapkiStrVec result = {0};
    size_t offs = 0;
    size_t pos = 0;
    size_t dsz = strlen(delim);
    while(true) {
        pos = TapkiStrFind(target, delim, offs);
        TapkiStr part = TapkiStrSub(ar, target, offs, pos);
        TapkiVecAppend(ar, &result, part);
        if (pos == Tapki_npos) {
            return result;
        }
        offs = pos + dsz;
    }
}

typedef struct __TapkiChunk {
    struct __TapkiChunk* next;
    size_t cap;
    char buff[];
} __TapkiChunk;

struct TapkiArena {
    size_t chunk_size;
    size_t ptr;
    __TapkiChunk* root;
    __TapkiChunk* current;
};

static void __TapkiArenaNext(TapkiArena* ar, size_t cap) {
    __TapkiChunk* next = ar->current ? ar->current->next : NULL;
    while (next) {
        ar->current = next;
        if (ar->current->cap > cap) return;
        next = ar->current->next;
    }
    next = (__TapkiChunk*)malloc(sizeof(__TapkiChunk) + cap);
    if (TAPKI_UNLIKELY(!next)) TapkiDie("arena.chunk.new");
    next->cap = cap;
    next->next = NULL;
    if (ar->current) ar->current->next = next;
    ar->current = next;
}

TapkiArena *TapkiArenaCreate(size_t chunkSize)
{
    TapkiArena* arena = (TapkiArena*)malloc(sizeof(TapkiArena));
    if (TAPKI_UNLIKELY(!arena)) TapkiDie("arena.new");
    *arena = (TapkiArena){};
    arena->chunk_size = chunkSize;
    __TapkiArenaNext(arena, chunkSize);
    arena->root = arena->current;
    return arena;
}

void *TapkiArenaAllocAligned(TapkiArena *arena, size_t size, size_t align)
{
    size_t tail = arena->ptr & (align - 1);
    size_t aligned = tail ? arena->ptr + align - tail : arena->ptr;
    size_t end = aligned + size;
    void* result;
    if (TAPKI_UNLIKELY(end > arena->current->cap)) {
        __TapkiArenaNext(arena, size > arena->chunk_size ? size : arena->chunk_size);
        arena->ptr = size;
        result = arena->current->buff;
    } else {
        arena->ptr = end;
        result = arena->current->buff + aligned;
    }
    if (size) memset(result, 0, size);
    return result;
}

void *TapkiArenaAlloc(TapkiArena *arena, size_t size)
{
    return TapkiArenaAllocAligned(arena, size, sizeof(void*)* 2);
}

char *TapkiArenaAllocChars(TapkiArena *arena, size_t count)
{
    return (char*)TapkiArenaAllocAligned(arena, count, 1);
}

void TapkiArenaClear(TapkiArena* arena)
{
    arena->ptr = 0;
    arena->current = arena->root;
}

void TapkiArenaFree(TapkiArena* arena)
{
    __TapkiChunk* curr = arena->root;
    while(curr) {
        __TapkiChunk* next = curr->next;
        free(curr);
        curr = next;
    }
    free(arena);
}

void* __tapki_vec_insert(TapkiArena* ar, void* _vec, size_t idx, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    assert(idx <= vec->size);
    __tapki_vec_reserve(ar, vec, vec->size + 1, tsz, al);
    size_t tail = vec->size++ - idx;
    if (tail) {
        char* src = vec->d + idx * tsz;
        char* dest = src + tsz;
        memmove(dest, src, tail * tsz);
        memset(src, 0, tsz);
    }
    if (tsz == 1) {
        vec->d[vec->size] = 0;
    }
    return vec->d + idx * tsz;
}

void* __tapki_map_at(TapkiArena *ar, void* _map, const void* key, const __tpk_map_info *info)
{
    __TapkiVec* map = (__TapkiVec*)_map;
    char *end = map->d + map->size * info->pair_sizeof;
    char *it = (char*)info->lower_bound(map->d, end, key);
    if (it == end) {
        char* pushed = (char*)__tapki_vec_push(ar, map, info->pair_sizeof, info->pair_align);
        memcpy(pushed, key, info->key_sizeof);
        return pushed + info->value_offset;
    } else if (info->eq(it, key)) {
        return it + info->value_offset;
    } else {
        size_t insert_at = (it - map->d) / info->pair_sizeof;
        char* inserted = (char*)__tapki_vec_insert(ar, map, insert_at, info->pair_sizeof, info->pair_align);
        memcpy(inserted, key, info->key_sizeof);
        return inserted + info->value_offset;
    }
}

void *__tapki_map_find(const void *_map, const void *key, const __tpk_map_info *info)
{
    const __TapkiVec* map = (const __TapkiVec*)_map;
    char *end = map->d + map->size * info->pair_sizeof;
    char *it = (char*)info->lower_bound(map->d, end, key);
    if (it != end && info->eq(it, key)) {
        return it + info->value_offset;
    } else {
        return NULL;
    }
}

bool __tapki_map_erase(void *_map, const void *key, const __tpk_map_info* info)
{
    __TapkiVec* map = (__TapkiVec*)_map;
    char *end = map->d + map->size * info->pair_sizeof;
    char *it = (char*)info->lower_bound(map->d, end, key);
    if (info->eq(it, key)) {
        size_t idx = (it - map->d) / info->pair_sizeof;
        __tapki_vec_erase(map, idx, info->pair_sizeof);
        return true;
    } else {
        return false;
    }
}

TapkiMapImplement(TapkiStrMap, TAPKI_STRING_LESS, TAPKI_STRING_EQ);

char *__tapki_vec_reserve(TapkiArena *ar, void *_vec, size_t count, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (tsz == 1)
        count++;
    if (vec->cap <= count) {
        uintptr_t arenaEnd = (uintptr_t)(ar->current->buff + ar->ptr);
        uintptr_t vecEnd = (uintptr_t)(vec->d + vec->cap * tsz);
        size_t ncap = vec->cap * 2;
        vec->cap = (ncap < count ? count : ncap);
        uintptr_t vecNewEnd = (uintptr_t)(vec->d + vec->cap * tsz);
        uintptr_t arenaCap = (uintptr_t)(ar->current->buff + ar->current->cap);
        // If we can just grow arena (vector is at the end of it) we do not relocate
        if (arenaEnd == vecEnd && vecNewEnd < arenaCap) {
            ar->ptr += (uintptr_t)(vecNewEnd - vecEnd);
        } else {
            char* newData = (char*)TapkiArenaAllocAligned(ar, vec->cap * tsz, al);
            _TAPKI_MEMCPY(newData, vec->d, vec->size * tsz);
            vec->d = newData;
        }
        if (tsz == 1 && vec->d)
            vec->d[count - 1] = 0;
    }
    return vec->d;
}

bool __tapki_vec_shrink(TapkiArena* ar, void* _vec, size_t tsz)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    uintptr_t arenaEnd = (uintptr_t)(ar->current->buff + ar->ptr);
    uintptr_t vecEnd = (uintptr_t)(vec->d + vec->cap * tsz);
    size_t diff = vec->cap - vec->size;
    if (diff && tsz == 1) {
        diff--;
    }
    if (arenaEnd == vecEnd) {
        ar->ptr -= tsz * diff;
        vec->cap -= diff;
        return true;
    } else {
        return false;
    }
}

char *__tapki_vec_resize(TapkiArena *ar, void *_vec, size_t count, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    char* data = __tapki_vec_reserve(ar, _vec, count, tsz, al);
    if (vec->size < count) {
        memset(data + vec->size * tsz, 0, tsz * (count - vec->size));
    }
    vec->size = count;
    return data;
}


TapkiStr* TapkiStrAppendVF(TapkiArena *ar, TapkiStr *str, const char *fmt, va_list list)
{
    va_list list2;
    va_copy(list2, list);
    char buff[200];
    const size_t cap = sizeof(buff);
    size_t count = vsnprintf(buff, cap, fmt, list);
    if (count) {
        size_t was = str->size;
        TapkiVecReserve(ar, str, str->size + count);
        if (count < cap) {
            if (count)
                memcpy(str->d + was, buff, count + 1);
        } else {
            vsnprintf(str->d + was, str->cap, fmt, list2);
        }
        str->size += count;
    }
    va_end(list2);
    return str;
}

TapkiStr TapkiVF(TapkiArena *ar, const char *fmt, va_list list)
{
    TapkiStr result = {0};
    TapkiStrAppendVF(ar, &result, fmt, list);
    return result;
}

TapkiStr* TapkiStrAppendF(TapkiArena *ar, TapkiStr *str, const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);
    TapkiStrAppendVF(ar, str, fmt, vargs);
    va_end(vargs);
    return str;
}

TapkiStr TapkiF(TapkiArena *ar, const char*__restrict__ fmt, ...)
{
    va_list vargs;
    va_start(vargs, fmt);
    TapkiStr result = TapkiVF(ar, fmt, vargs);
    va_end(vargs);
    return result;
}

TapkiStr TapkiS(TapkiArena *ar, const char *s)
{
    size_t len = strlen(s);
    TapkiStr result = __tapkis_withn(ar, len);
    _TAPKI_MEMCPY(result.d, s, len);
    return result;
}

static FILE* __tpk_open(const char* file, const char* mode, const char* comment) {
    FILE* f = fopen(file, mode);
    if (!f) {
        TapkiDie("Could not open for %s: %s => [Errno: %d] %s\n", comment, file, errno, strerror(errno));\
    }
    return f;
}

void TapkiWriteFile(const char *file, const char *contents)
{
    FILE* f = __tpk_open(file, "wb", "write");
    fwrite(contents, strlen(contents), 1, f);
    fclose(f);
}

void TapkiAppendFile(const char *file, const char *contents)
{
    FILE* f = __tpk_open(file, "ab", "append");
    fwrite(contents, strlen(contents), 1, f);
    fclose(f);
}

double TapkiToFloat(const char *s)
{
    char* end;
    double res = strtod(s, &end);
    if (TAPKI_UNLIKELY(*end != 0)) {
        TapkiDie("Could fully not convert to double: %s", s);
    }
    return res;
}

uint64_t TapkiToU64(const char *s)
{
    char* end;
    uint64_t res = strtoull(s, &end, 10);
    if (TAPKI_UNLIKELY(*end != 0)) {
        TapkiDie("Could fully not convert to UInt64: %s", s);
    }
    return res;
}

int64_t TapkiToI64(const char *s)
{
    char* end;
    int64_t res = strtoll(s, &end, 10);
    if (TAPKI_UNLIKELY(*end != 0)) {
        TapkiDie("Could fully not convert to Int64: %s", s);
    }
    return res;
}

uint32_t TapkiToU32(const char *s)
{
    uint64_t res = TapkiToI64(s);
    if (TAPKI_UNLIKELY(res > UINT32_MAX)) {
        TapkiDie("Integer is too big for UInt32: %s", s);
    }
    return (uint32_t)res;
}

int32_t TapkiToI32(const char *s)
{
    int64_t res = TapkiToI64(s);
    if (TAPKI_UNLIKELY(res > INT32_MAX)) {
        TapkiDie("Integer is too big for Int32: %s", s);
    }
    return (int32_t)res;
}

typedef struct {
    const TapkiCLI* orig;
    const char* metavar;
    const char* firstAlias;
    int firstAliasDashes;
    int hits;
} __tpk_cli_priv;

typedef struct {
    __tpk_cli_priv* info;
    const char* alias;
    bool is_long;
} __tpk_cli_spec;

typedef struct {
    const char* name;
    __tpk_cli_spec value;
} __tpk_cli_named;

typedef TapkiVec(__tpk_cli_named) __tpk_cli_named_vec;

static __tpk_cli_spec* __tpk_cli_named_Find(__tpk_cli_named_vec* vec, const char* name) {
    TapkiVecForEach(vec, it) {
        if (TAPKI_STRING_EQ(it->name, name)) {
            return &it->value;
        }
    }
    return NULL;
}

static __tpk_cli_spec* __tpk_cli_named_At(Arena* arena, __tpk_cli_named_vec* vec, const char* name) {
    __tpk_cli_spec* res = __tpk_cli_named_Find(vec, name);
    if (!res) {
        __tpk_cli_named* pushed = TapkiVecPush(arena, vec);
        pushed->name = name;
        res = &pushed->value;
    }
    return res;
}

typedef struct {
    __tpk_cli_named_vec named;
    TapkiVec(__tpk_cli_spec) pos;
    TapkiVec(__tpk_cli_priv) _storage;
    bool need_help;
    TapkiCLI help_opt;
    TapkiCLI prog_opt;
} __tpk_cli_context;

static const char* __tpk_cli_dashes(const char* name, size_t *dashes)
{
    *dashes = 0;
    while(*name == '-') {
        (*dashes)++; name++;
    }
    TapkiAssert(*dashes == 0 || *dashes == 1 || *dashes == 2);
    return name;
}

static void __tpk_cli_clean(TapkiArena *ar, __tpk_cli_context* ctx, const TapkiCLI cli[])
{
    size_t count = 0;
    const TapkiCLI* curr = cli;
    while(curr->name) {
        count++;
        curr++;
    }
    TapkiVecResize(ar, &ctx->_storage, count);
    curr = cli;
}

static void __tpk_cli_reset(TapkiArena *ar, __tpk_cli_context* ctx)
{
    TapkiVecForEach(&ctx->_storage, it) {
        it->hits = 0;
    }
}

static void __tpk_cli_init(TapkiArena *ar, __tpk_cli_context* ctx, const TapkiCLI _source_spec[])
{
    ctx->need_help = false;
    ctx->help_opt = (TapkiCLI){"-h,--help", &ctx->need_help, .flag=true, .help="show this help message and exit"};
    {
        size_t count = 0;
        const TapkiCLI* curr = _source_spec;
        while(curr->name || curr->program) {
            count++;
            curr++;
        }
        TapkiVecReserve(ar, &ctx->_storage, count + 1);
        ctx->_storage.size = count;
        curr = _source_spec;
        TapkiVecForEach(&ctx->_storage, it) {
            *it = (__tpk_cli_priv){curr, curr->metavar};
            curr++;
        }
        TapkiVecAppend(ar, &ctx->_storage, (__tpk_cli_priv){&ctx->help_opt});
    }
    TapkiArena* temp = TapkiArenaCreate(256);
    TapkiVecForEach(&ctx->_storage, it) {
        FrameF("Parse CLI argument spec (#%zu): %s", it->orig - _source_spec, it->orig->name) {
            if (it->orig->flag && it->orig->metavar) {
                Die("'flag' options cannot have 'metavar' name");
            }
            if (it->orig->program) {
                if (ctx->prog_opt.program) {
                    Die("Program option already specified");
                }
                if (it->orig->data) {
                    Die("Program option cannot have pointer to data");
                }
                ctx->prog_opt = *it->orig;
                continue;
            }
            for (__tpk_cli_priv* check = it + 1; check != ctx->_storage.d + ctx->_storage.size; ++check) {
                if (check->orig->data == it->orig->data) {
                    Die("Conflict: output &pointer already used in argument: %s", check->orig->name);
                }
            }
            TapkiAssert(it->orig->data != NULL && "Pointer to variable missing");
            char *saveptr = NULL;
            char *names = TapkiS(ar, it->orig->name).d;
            size_t dashes;
            char* alias;
            bool was_pos = false;
            bool was_named = false;
            bool last_pos_required = false;
            while((alias = strtok_r(saveptr ? NULL : names, ",", &saveptr))) {
                alias = (char*)__tpk_cli_dashes(alias, &dashes);
                if (!it->firstAlias) {
                    it->firstAlias = alias;
                }
                if (!it->orig->flag && !it->metavar) {
                    it->metavar = TapkiF(ar, "<%s>", it->firstAlias).d;
                }
                if (dashes > 2) {
                    Die("Unexpected '-' count: %zu", dashes);
                }
                if (dashes) {
                    if (it->orig->flag && it->orig->required) {
                        Die("'flag' arguments cannot be also 'required'");
                    }
                    was_named = true;
                    __tpk_cli_spec* named = __tpk_cli_named_At(ar, &ctx->named, alias);
                    *named = (__tpk_cli_spec){it, alias};
                    named->is_long = dashes == 2;
                    if (!it->firstAliasDashes) {
                        it->firstAliasDashes = dashes;
                    }
                } else {
                    if (was_pos) {
                        Die("Positional arguments cannot have aliases (e.g. 'name1,name2')");
                    }
                    was_pos = true;
                    if (it->orig->flag) {
                        Die("Positional arguments cannot be flags");
                    }
                    if (ctx->pos.size && TapkiVecAt(&ctx->pos, ctx->pos.size - 1)->info->orig->many) {
                        Die("Positional argument cannot follow another with 'many' flag (it will consume all)");
                    }
                    bool req = it->orig->required;
                    if (!last_pos_required && req) {
                        Die("Cannot have 'required' positional argument after 'non-required'");
                    }
                    last_pos_required = req;
                    *TapkiVecPush(ar, &ctx->pos) = (__tpk_cli_spec){it, alias};
                }
                if (was_named && was_pos) {
                    Die("CLI Spec has '--named' AND 'pos' arguments");
                }
            }
        }
    }
    TapkiArenaFree(temp);
}

static bool __tpk_cli_parse(TapkiArena *ar, __tpk_cli_spec* spec, int i, const char* arg, TapkiStr* err) {
    spec->info->hits++;
    const TapkiCLI* cli = spec->info->orig;
    void* output = cli->data;
    if (cli->many) {
        if (cli->int64) output = TapkiVecPush(ar, (TapkiIntVec*)output);
        else output = TapkiVecPush(ar, (TapkiStrVec*)output);
    }
    TapkiStr str = TapkiS(ar, arg);
    if (cli->int64) {
        char* end;
        *(int64_t*)output = strtoll(arg, &end, 10);
        if (TAPKI_UNLIKELY(*end != 0)) {
            *err = TapkiF(ar, "argument (#%d): %s: Could fully not convert to Int64", i, arg);
            return false;
        }
    } else {
        *(TapkiStr*)output = str;
    }
    return true;
}

static TapkiCLIVarsResult __TapkiCLI_ParseVars(TapkiArena *ar, __tpk_cli_context* ctx, int argc, char** argv)
{
    TapkiCLIVarsResult result = {0};
    __tpk_cli_spec* named = NULL;
    __tpk_cli_spec* pos = ctx->pos.d;
    __tpk_cli_spec* pos_end = ctx->pos.d + ctx->pos.size;
    size_t posIndex = 0;
    bool ignoreNamed = false;
    for(int i = 1; i < argc; ++i) {
        const char* orig_arg = argv[i];
        if (TAPKI_STRING_EQ(orig_arg, "--")) {
            ignoreNamed = true;
            continue;
        }
        if (named) {
        parse_named:
            if (!__tpk_cli_parse(ar, named, i, orig_arg, &result.error)) {
                return result;
            }
            named = NULL;
            continue;
        }
        size_t dashes;
        const char* stripped_arg = __tpk_cli_dashes((char*)orig_arg, &dashes);
        if (ignoreNamed || dashes == 0) {
            if (pos == pos_end) {
                result.error = TapkiF(ar, "unexpected positional argument (#%d): %s", i, stripped_arg);
                return result;
            }
            if (!__tpk_cli_parse(ar, pos, i, orig_arg, &result.error)) {
                return result;
            }
            if (!pos->info->orig->many) {
                pos++;
            }
        } else if (dashes < 3) {
            const char* eq = strchr(stripped_arg, '=');
            char* lookup = (char*)stripped_arg;
            if (eq) {
                stripped_arg = eq + 1;
                lookup = TapkiS(ar, stripped_arg).d;
                lookup[eq - stripped_arg] = 0;
            }
            named = __tpk_cli_named_Find(&ctx->named, stripped_arg);
            if (!named) {
                result.error = TapkiF(ar, "unknown argument (#%d): %s", i, orig_arg);
                return result;
            }
            bool is_two = dashes == 2;
            if (named->is_long != is_two) {
                const char* expected = named->is_long ? "--" : "-";
                result.error = TapkiF(ar, "unknown argument (#%d): %s. Expected prefix: %s", i, orig_arg, expected);
                return result;
            }
            if (named->info->hits && !named->info->orig->many) {
                result.error = TapkiF(ar, "argument (#%d): %s: more than one value provided", i, orig_arg);
                return result;
            }
            if (named->info->orig->flag) {
                *(bool*)named->info->orig->data = true;
                if (TAPKI_UNLIKELY(eq)) {
                    result.error = TapkiF(ar,
                        "argument (#%d): %s: cannot assign with '=' to 'flag' named argument", i, orig_arg);
                    return result;
                }
                named = NULL;
            }
            if (eq) {
                goto parse_named;
            }
        } else {
            result.error = TapkiF(ar, "unexpected amount of '-' in argument (#%d): %s", i, orig_arg);
            return result;
        }
    }
    if (named) {
        result.error = TapkiF(ar, "expected value for: %s", named->info->orig->name);
        return result;
    }
    VecForEach(&ctx->_storage, spec) {
        if (spec->orig->required && !spec->hits) {
            result.error = TapkiF(ar, "missing argument: %s", spec->orig->name);
            return result;
        }
    }
    result.ok = true;
    result.need_help = ctx->need_help;
    return result;
}

#ifdef TAPKI_CLI_NO_TTY
static size_t __tpk_term_width() { return 0; }
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static size_t __tpk_term_width() {
    DWORD mode;
    HANDLE herr = GetStdHandle(STD_ERROR_HANDLE);
    if (!GetConsoleMode(herr, &mode)) return 0;
    mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    (void)SetConsoleMode(herr, mode);
    CONSOLE_FONT_INFO info;
    GetCurrentConsoleFont(herr, FALSE, &info);
    return info.dwFontSize.X;
}
#else
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
static size_t __tpk_term_width() {
    if (!isatty(STDERR_FILENO)) return 0;
    struct winsize w;
    ioctl(STDERR_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}
#endif

// usage: prog [-h] [--long <long>] pos
static TapkiStr __TapkiCLI_Usage(TapkiArena *ar, __tpk_cli_context* ctx, int argc, char **argv)
{
    size_t offset = TapkiStrRevFind(argv[0], __tpk_sep);
    TapkiStr result = {0};
    if (ctx->prog_opt.name) {
        result = TapkiS(ar, ctx->prog_opt.name);
    } else {
        result = TapkiS(ar, argv[0] + (offset == Tapki_npos ? 0 : offset + 1));
    }
    TapkiVecForEach(&ctx->named, named) {
        const TapkiCLI* cli = named->value.info->orig;
        if (named->value.info->hits++) continue;
        const char* dashes = named->value.info->firstAliasDashes == 2 ? "--" : "-";
        const char* metavar = named->value.info->metavar;
        const char* dots = cli->many ? "..." : "";
        const char* lbracket = cli->required ? "" : "[";
        const char* rbracket = cli->required ? "" : "]";
        TapkiStrAppend(ar, &result, " ", lbracket, dashes, named->value.info->firstAlias);
        if (metavar) {
            TapkiStrAppend(ar, &result, " ", metavar);
        }
        TapkiStrAppend(ar, &result, rbracket, dots);
    }
    TapkiVecForEach(&ctx->pos, pos) {
        const TapkiCLI* cli = pos->info->orig;
        const char* lbracket = cli->required ? "" : "[";
        const char* rbracket = cli->required ? "" : "]";
        const char* dots = cli->many ? "..." : "";
        TapkiStrAppend(ar, &result, " ", lbracket, pos->info->metavar ? pos->info->metavar : pos->info->firstAlias, rbracket, dots);
    }
    return result;
}

/*
positional arguments:
pos                   pos arg help...
                      wrap...
[longest............] [until termw................]
options:           [__]
-h, --help            show this help message and exit
--long LONG, -l LONG  named arg (not flag) help
*/

static void __tpk_cli_wrapping_help(size_t pad, size_t termw, TapkiArena *ar, TapkiStr* out, const char* args, const char* help)
{
    help = help ? help : "";
    size_t rlen = 0; int wrap = 0;
again:
    rlen = strlen(help);
    if (termw && (rlen + pad + 3) > termw) {
        int diff = termw - (pad + 3);
        wrap = diff > 0 ? diff : 10;
    } else {
        wrap = (int)rlen;
    }
    TapkiStrAppendF(ar, out, "  %-*s  %.*s\n", (int)pad, args, wrap, help);
    if (rlen != wrap) {
        help += wrap;
        args = "";
        goto again;
    }
}


static TapkiStr __TapkiCLI_Help(TapkiArena *_ar, __tpk_cli_context* ctx)
{
    TapkiArena* temp = TapkiArenaCreate(2048);
    typedef struct {
        const char* args;
        size_t alen;
        const char* help;
    } __tpk_help_pair;
    TapkiVec(__tpk_help_pair) pos_pairs = {0};
    TapkiVec(__tpk_help_pair) named_pairs = {0};
    size_t termw = __tpk_term_width();
    TapkiStr result = {0};
    if (ctx->prog_opt.help) {
        TapkiStrAppend(temp, &result, ctx->prog_opt.help, "\n\n");
    }
    TapkiVecForEach(&ctx->pos, pos) {
        *TapkiVecPush(temp, &pos_pairs) = (__tpk_help_pair){pos->alias, strlen(pos->alias), pos->info->orig->help};
    }
    {
        TapkiStr lastNamedArg = {0};
        const char* lastHelp = "";
        TapkiVecForEach(&ctx->named, named) {
            __tpk_cli_spec* spec = &named->value;
            if (!spec->info->hits++) {
                if (lastNamedArg.size) {
                    *TapkiVecPush(temp, &named_pairs) = (__tpk_help_pair){lastNamedArg.d, lastNamedArg.size, lastHelp};
                    lastNamedArg = (TapkiStr){0};
                }
                lastHelp = spec->info->orig->help;
            } else {
                TapkiStrAppend(temp, &lastNamedArg, ", ");
            }
            const char* dash = named->value.is_long ? "--" : "-";
            TapkiStrAppend(temp, &lastNamedArg, dash, spec->alias);
            if (spec->info->metavar) {
                TapkiStrAppend(temp, &lastNamedArg, " ", spec->info->metavar);
            }
        }
        if (lastNamedArg.size) {
            *TapkiVecPush(temp, &named_pairs) = (__tpk_help_pair){lastNamedArg.d, lastNamedArg.size, lastHelp};
        }
    }
    int pad = 0;
    {
        TapkiVecForEach(&pos_pairs, pos) {
            if (pos->alen > pad) pad = (int)pos->alen;
        }
        TapkiVecForEach(&named_pairs, named) {
            if (named->alen > pad) pad = (int)named->alen;
        }
    }
    pad += 2; // 2 spaces
    TapkiStrAppend(temp, &result, "positional arguments:\n");
    TapkiVecForEach(&pos_pairs, pos) {
        __tpk_cli_wrapping_help(pad, termw, temp, &result, pos->args, pos->help);
    }
    TapkiStrAppend(temp, &result, "\noptions:\n");
    TapkiVecForEach(&named_pairs, named) {
        __tpk_cli_wrapping_help(pad, termw, temp, &result, named->args, named->help);
    }
    TapkiStr final_result = TapkiStrCopy(_ar, result.d, result.size);
    TapkiArenaFree(temp);
    return final_result;
}

TapkiStr TapkiCLI_Usage(TapkiArena *ar, const TapkiCLI cli[], int argc, char **argv)
{
    __tpk_cli_context ctx = {0};
    __tpk_cli_init(ar, &ctx, cli);
    return __TapkiCLI_Usage(ar, &ctx, argc, argv);
}

TapkiStr TapkiCLI_Help(TapkiArena *ar, const TapkiCLI cli[])
{
    __tpk_cli_context ctx = {0};
    __tpk_cli_init(ar, &ctx, cli);
    return __TapkiCLI_Help(ar, &ctx);
}

TapkiCLIVarsResult TapkiCLI_ParseVars(TapkiArena *ar, const TapkiCLI cli[], int argc, char** argv)
{
    __tpk_cli_context ctx = {0};
    __tpk_cli_init(ar, &ctx, cli);
    return __TapkiCLI_ParseVars(ar, &ctx, argc, argv);
}

int TapkiParseCLI(TapkiArena *ar, TapkiCLI cli[], int argc, char **argv)
{
    __tpk_cli_context ctx = {0};
    __tpk_cli_init(ar, &ctx, cli);
    TapkiCLIVarsResult result = __TapkiCLI_ParseVars(ar, &ctx, argc, argv);
    if (!result.ok) {
        __tpk_cli_reset(ar, &ctx);
        fprintf(stderr, "usage: %s\n", __TapkiCLI_Usage(ar, &ctx, argc, argv).d);
        fprintf(stderr, "%s: error: %s\n", argv[0], result.error.d);
        return 1;
    }
    if (result.need_help) {
        __tpk_cli_reset(ar, &ctx);
        fprintf(stderr, "usage: %s\n\n", __TapkiCLI_Usage(ar, &ctx, argc, argv).d);
        __tpk_cli_reset(ar, &ctx);
        fprintf(stderr, "%s\n", __TapkiCLI_Help(ar, &ctx).d);
        return 1;
    }
    return 0;
}

TapkiStr TapkiReadFile(TapkiArena *ar, const char *file)
{
    FILE* f = __tpk_open(file, "rb", "read");
    if (fseek(f, 0, SEEK_END))
        TapkiDie("Could not seek file: %s => [Errno: %d] %s\n", file, errno, strerror(errno));
    TapkiStr str = __tapkis_withn(ar, ftell(f));
    rewind(f);
    int e = fread(str.d, str.size, 1, f);
    (void)(e = e);
    fclose(f);
    return str;
}

TapkiStr __tpk_path_join(TapkiArena *ar, const char **parts, size_t count)
{
    TapkiStr res = {0};
    for (size_t i = 0; i < count; ++i) {
        const char* part = parts[i];
        if (i) {
            TapkiVecAppend(ar, &res, *__tpk_sep);
        }
        TapkiStrAppend(ar, &res, part);
    }
    return res;
}


#undef _TAPKI_MEMCPY

#ifdef __cplusplus
}
#endif
#endif //TAPKI_IMPLEMENTATION
