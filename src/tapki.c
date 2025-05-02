#include "tapki.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _TAPKI_MEM(dest, src, count) if (src && count) memcpy(dest, src, count)
#define _TAPKI_CSTR(s) (_TAPKI_UNLIKELY(!s) ? "" : s)

int8_t *__tapki_vec_reserve(TapkiArena *ar, void *_vec, size_t count, size_t tsz, size_t al)
{
    if (tsz == 1)
        count++;
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (vec->cap <= count) {
        size_t ncap = vec->cap * 2;
        vec->cap = (ncap < count ? count : ncap);
        int8_t* newData = (int8_t*)ArenaAllocAligned(ar, vec->cap * tsz, al);
        _TAPKI_MEM(newData, vec->data, vec->size * tsz);
        vec->data = newData;
        if (_TAPKI_UNLIKELY(!vec->data)) TapkiDie("vector.reserve");
    }
    if (tsz == 1 && vec->data)
        vec->data[count - 1] = 0;
    return vec->data;
}

void __tapki_vec_erase(void *_vec, size_t idx, size_t tsz)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (_TAPKI_UNLIKELY(vec->size <= idx)) TapkiDie("vector.erase");
    size_t tail = (--vec->size) - idx;
    if (tail) {
        memmove(vec->data + idx * tsz, vec->data + (idx + 1) * tsz, tail * tsz);
    }
    if (tsz == 1 && vec->data)
        vec->data[vec->size] = 0;
}

void TapkiDie(const char *msg)
{
    fprintf(stderr, "TAPKI: FATAL: %s", msg);
    exit(1);
}

void TapkiVecClear(void *_vec)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    vec->size = 0;
}

static TapkiString __tapkis_withn(TapkiArena* ar, size_t res) {
    TapkiString str = {0};
    TapkiVecReserve(ar, &str, res);
    str.size = str.cap = res;
    str.data[res] = 0;
    return str;
}

static TapkiString __tapkis_strv(TapkiArena* ar, const char* s, size_t n) {
    TapkiString res = __tapkis_withn(ar, n);
    _TAPKI_MEM(res.data, s, n);
    return res;
}


void __tapki_vec_append(TapkiArena *ar, void *_vec, void *data, size_t count, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    __tapki_vec_reserve(ar, vec, vec->size + count, tsz, al);
    _TAPKI_MEM(vec->data + vec->size * tsz, data, count * tsz);
    vec->size += count;
}

TapkiString* __tapkis_append(TapkiArena *ar, TapkiString *target, const char** strs, size_t count)
{
    size_t total = 0;
    size_t lens[count];
    for (size_t i = 0; i < count; ++i) {
        total += (lens[i] = strlen(strs[i]));
    }
    TapkiVecReserve(ar, target, target->size + total + 1);
    for (size_t i = 0; i < count; ++i) {
        void* out = target->data + target->size;
        _TAPKI_MEM(out, strs[i], lens[i]);
        target->size += lens[i];
    }
    target->data[target->size] = 0;
    return target;
}

TapkiString __tapkis_itos(TapkiArena *ar, long long i)
{
    char buff[200];
    return __tapkis_strv(ar, buff, sprintf(buff, "%lli", i));
}

TapkiString __tapkis_utos(TapkiArena *ar, unsigned long long u)
{
    char buff[200];
    return __tapkis_strv(ar, buff, sprintf(buff, "%llu", u));
}

TapkiString __tapkis_ftos(TapkiArena *ar, double f)
{
    char buff[200];
    return __tapkis_strv(ar, buff, sprintf(buff, "%lf", f));
}

TapkiString __tapkis_stos(TapkiArena *ar, const char *s)
{
    size_t n = strlen(s);
    TapkiString res = __tapkis_withn(ar, n);
    _TAPKI_MEM(res.data, s, n);
    return res;
}

TapkiString __tapkis_pcopy(TapkiArena *ar, const TapkiString *str)
{
    TapkiString res = __tapkis_withn(ar, str->size);
    _TAPKI_MEM(res.data, str->data, str->size);
    return res;
}

TapkiString TapkiStringSub(TapkiArena *ar, const char *target, size_t from, size_t to)
{
    assert(to >= from);
    if (!target) target = "";
    size_t tsz = strlen(target);
    assert(from <= tsz);
    to = to > tsz ? tsz : to;
    TapkiString res = {0};
    size_t diff = (size_t)(to - from);
    TapkiVecReserve(ar, &res, diff);
    _TAPKI_MEM(res.data, target + from, diff);
    return res;
}

size_t TapkiStringFind(const char *target, const char *what, size_t offset)
{
    const char* found = strstr(target + offset, what);
    if (!found) return TapkiNpos;
    return (size_t)(found - target);
}

TapkiStringVec TapkiStringSplit(TapkiArena *ar, const char *target, const char *delim)
{
    if (!target) target = "";
    if (!delim) delim = "";
    TapkiStringVec result = {0};
    size_t offs = 0;
    size_t pos = 0;
    size_t dsz = strlen(delim);
    while(true) {
        pos = TapkiStringFind(target, delim, offs);
        TapkiString part = TapkiStringSub(ar, target, offs, pos);
        TapkiVecAppend(ar, &result, part);
        if (pos == TapkiNpos) {
            return result;
        }
        offs = pos + dsz;
    }
}

void TapkiStringErase(TapkiString *target, size_t index)
{
    TapkiVecErase(target, index);
    target->data[target->size] = 0;
}

#undef _TAPKI_CSTR
#undef _TAPKI_MEM

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
    while (ar->current) {
        __TapkiChunk* next = ar->current->next;
        if (!next) break;
        ar->current = next;
        if (next->cap > cap) {
            return;
        }
    }
    __TapkiChunk* chunk = (__TapkiChunk*)malloc(sizeof(__TapkiChunk) + cap);
    if (_TAPKI_UNLIKELY(!chunk)) TapkiDie("arena.chunk.new");
    chunk->cap = cap;
    if (ar->current) ar->current->next = chunk;
    ar->current = chunk;
    ar->current->next = NULL;
}

TapkiArena *TapkiArenaCreate(size_t chunkSize)
{
    TapkiArena* arena = (TapkiArena*)malloc(sizeof(TapkiArena));
    if (_TAPKI_UNLIKELY(!arena)) TapkiDie("arena.new");
    *arena = (TapkiArena){};
    arena->chunk_size = chunkSize;
    __TapkiArenaNext(arena, chunkSize);
    arena->root = arena->current;
    return arena;
}

static void* __tapki_zero(void* ptr, size_t size) {
    memset(ptr, 0, size);
    return ptr;
}

void *TapkiArenaAllocAligned(TapkiArena *arena, size_t size, size_t align)
{
    if (_TAPKI_UNLIKELY(size > arena->chunk_size)) {
        __TapkiArenaNext(arena, size);
        arena->ptr = size;
        return __tapki_zero(arena->current->buff, size);
    }
    size_t tail = arena->ptr & (align - 1);
    size_t aligned = tail ? arena->ptr + align - tail : arena->ptr;
    size_t end = aligned + size;
    if (_TAPKI_UNLIKELY(end > arena->current->cap)) {
        __TapkiArenaNext(arena, arena->chunk_size);
        arena->ptr = size;
        return __tapki_zero(arena->current->buff, size);
    } else {
        arena->ptr = end;
        return __tapki_zero(arena->current->buff + aligned, size);
    }
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

#ifdef __cplusplus
}
#endif
