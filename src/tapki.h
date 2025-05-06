#pragma once
#ifndef TAPKI_H
#define TAPKI_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
    #define _TAPKI_NORETURN __attribute__((noreturn))
    #define _TAPKI_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define _TAPKI_ALLOC_ATTR(sz, al) __attribute__((malloc, alloc_size(sz), alloc_align(al)))
    #define _TAPKI_FMT_ATTR(fmt, args) __attribute__((format(printf, fmt, args)))
#else
    #if defined(_MSC_VER)
        #define _TAPKI_NORETURN __declspec(noreturn)
    #else
        #define _TAPKI_NORETURN
    #endif
    #define _TAPKI_UNLIKELY(x) x
    #define _TAPKI_ALLOC_ATTR(sz, al)
    #define _TAPKI_FMT_ATTR(fmt, args)
#endif

// If TAPKI_FULL_NAMESPACE is not defined -> you can use Public API without Tapki prefix

_TAPKI_NORETURN void TapkiDie(const char* msg);
_TAPKI_FMT_ATTR(1, 2) _TAPKI_NORETURN void TapkiDieF(const char* __restrict__ fmt, ...);

typedef struct TapkiArena TapkiArena;

TapkiArena* TapkiArenaCreate(size_t chunkSize);
_TAPKI_ALLOC_ATTR(2, 3) void* TapkiArenaAllocAligned(TapkiArena* arena, size_t size, size_t align);
void* TapkiArenaAlloc(TapkiArena* arena, size_t size);
char* TapkiArenaAllocChars(TapkiArena* arena, size_t count);
void TapkiArenaClear(TapkiArena* arena);
void TapkiArenaFree(TapkiArena* arena);

#define TapkiVec(type) struct { type* data; size_t size; size_t cap; }
#define TapkiVecT(vec) __typeof__(*(vec)->data)
#define TapkiVecS(vec) sizeof(*(vec)->data)
#define TapkiVecA(vec) _Alignof(TapkiVecT(vec))
#define TapkiVecSA(vec) TapkiVecS(vec), TapkiVecA(vec)
#define TapkiVecAppend(arena, vec, ...) __tapki_vec_append((arena), (vec), __TapkiArr(TapkiVecT(vec), __VA_ARGS__), TapkiVecSA(vec))
#define TapkiVecPush(arena, vec) ((TapkiVecT(vec)*)__tapki_vec_push((arena), (vec), TapkiVecSA(vec)))
#define TapkiVecPop(vec)  *((TapkiVecT(vec)*)__tapki_vec_pop((vec)))
#define TapkiVecAt(vec, idx)  ((TapkiVecT(vec)*)__tapki_vec_at((vec), idx))[idx]
#define TapkiVecErase(vec, idx)  __tapki_vec_erase((vec), idx, TapkiVecS(vec))
#define TapkiVecInsert(arena, vec, idx)  ((TapkiVecT(vec)*)__tapki_vec_insert((arena), (vec), idx, TapkiVecSA(vec)))
#define TapkiVecForEach(vec, it) for (TapkiVecT(vec)* it = (vec)->data; it && it != (vec)->data + (vec)->size; ++it)
void    TapkiVecClear(void* _vec);
#define TapkiVecReserve(arena, vec, n) __tapki_vec_reserve((arena), (vec), n, TapkiVecSA(vec))

typedef TapkiVec(char) TapkiStr;
typedef struct { const TapkiStr key; TapkiStr value; } TapkiStrPair;
typedef TapkiVec(TapkiStr) TapkiStrVec;
typedef TapkiVec(TapkiStrPair) TapkiStrMap;

#define Tapki_npos ((size_t)-1)
#define TapkiStringAppend(arena, s, ...) __tapkis_append((arena), (s), __TapkiArr(const char*, __VA_ARGS__))

TapkiStr TapkiStrSub(TapkiArena *ar, const char* target, size_t from, size_t to);
size_t TapkiStrFind(const char* target, const char* what, size_t offset);
TapkiStrVec TapkiStrSplit(TapkiArena *ar, const char* target, const char *delim);
_TAPKI_FMT_ATTR(2, 3) TapkiStr TapkiF(TapkiArena* ar, const char* __restrict__ fmt, ...);
TapkiStr TapkiVF(TapkiArena* ar, const char* __restrict__ fmt, va_list list);
TapkiStr TapkiS(TapkiArena* ar, const char* s);

TapkiStr* TapkiStrMapAt(TapkiArena *ar, TapkiStrMap* map, const char* key);
void TapkiStrMapErase(TapkiStrMap* map, const char* key);

TapkiStr TapkiReadFile(TapkiArena* ar, const char* file);
void TapkiWriteFile(const char* file, const char* contents);
void TapkiAppendFile(const char* file, const char* contents);

typedef struct TapkiCLI {
    bool ok;
    TapkiStr error;
    TapkiStrVec positional;
    TapkiStrMap values;
} TapkiCLI;

TapkiCLI TapkiParseCLI(TapkiArena *ar, int argc, const char* const* argv);

#ifndef TAPKI_FULL_NAMESPACE

typedef TapkiArena Arena;
typedef TapkiStr Str;
typedef TapkiStrPair StrPair;
typedef TapkiStrMap StrMap;
typedef TapkiStrVec StrVec;
typedef TapkiCLI CLI;

#define Vec(type)                       TapkiVec(type)
#define VecAppend(arena, vec, ...)      TapkiVecAppend(arena, vec, ##__VA_ARGS__)
#define VecPush(arena, vec)             TapkiVecPush(arena, vec)
#define VecPop(vec)                     TapkiVecPop(vec)
#define VecAt(vec, idx)                 TapkiVecAt(vec, idx)
#define VecForEach(vec, it)             TapkiVecForEach(vec, it)
#define VecErase(vec, idx)              TapkiVecErase(vec, idx)
#define VecInsert(arena, vec, idx)      TapkiVecInsert(arena, vec, idx)
#define VecClear(vec)                   TapkiVecClear(vec)
#define VecReserve(arena, vec, n)       TapkiVecReserve(arena, vec, n)
#define ArenaCreate(chunksize)          TapkiArenaCreate(chunksize)
#define ArenaAllocAligned(ar, sz, al)   TapkiArenaAllocAligned(ar, sz, al)
#define ArenaAlloc(arena, sz)           TapkiArenaAlloc(arena, sz)
#define ArenaClear(arena)               TapkiArenaClear(arena)
#define ArenaFree(arena)                TapkiArenaFree(arena)
#define F(arena, fmt, ...)              TapkiF(arena, fmt, ##__VA_ARGS__)
#define S(arena, str)                   TapkiS(arena, str)
#define StrSplit(arena, s, delim)       TapkiStrSplit(arena, s, delim)
#define StrSub(arena, s, from, to)      TapkiStrSub(arena, s, from, to)
#define StrFind(s, needle, offs)        TapkiStrFind(s, needle, offs)
#define StrMapAt(arena, map, key)       TapkiStrMapAt(arena, map, key)
#define StrMapErase(map, key)           TapkiStrMapErase(arena, map, key)
#define npos                            Tapki_npos
#define Die(msg)                        TapkiDie(msg)
#define DieF(fmt, ...)                  TapkiDieF(fmt, ##__VA_ARGS__)
#define WriteFile(file, data)           TapkiWriteFile(file, data)
#define AppendFile(file, data)          TapkiAppendFile(file, data)
#define ReadFile(arena, file)           TapkiReadFile(arena, file)
#define ParseCLI(arena, argc, argv)     TapkiParseCLI(arena, argc, argv)

#endif












// --- Private stuff

#define __TapkiArr(t, ...) (t[]){__VA_ARGS__}, PP_NARG(__VA_ARGS__)

typedef struct {
    char* data;
    size_t size;
    size_t cap;
} __TapkiVec;

void* __tapki_vec_insert(TapkiArena* ar, void* _vec, size_t idx, size_t tsz, size_t al);
char* __tapki_vec_reserve(TapkiArena* ar, void* _vec, size_t count, size_t tsz, size_t al);
void __tapki_vec_append(TapkiArena* ar, void* _vec, void* data, size_t count, size_t tsz, size_t al);
TapkiStr* __tapkis_append(TapkiArena *ar, TapkiStr* target, const char **src, size_t count);
void __tapki_vec_erase(void* _vec, size_t idx, size_t tsz);

static inline void* __tapki_vec_push(TapkiArena* ar, void* _vec, size_t tsz, size_t al) {
    __TapkiVec* vec = (__TapkiVec*)_vec;
    return (vec->size >= vec->cap ? __tapki_vec_reserve(ar, vec, vec->cap + 1, tsz, al) : vec->data) + (vec->size++ * tsz);
}

static inline void* __tapki_vec_pop(void* _vec, size_t tsz) {
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (!vec->size) TapkiDie("vector.pop: Vector is empty");
    return vec->data + (vec->size-- * tsz);
}

static inline void* __tapki_vec_at(void* _vec, size_t pos, size_t tsz) {
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (vec->size <= pos) TapkiDieF("vector.at: index(%zu) > size(%zu)", pos, vec->size);
    return vec->data;
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



