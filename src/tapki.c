#include "tapki.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _TAPKI_MEMSET(dest, src, count) if (src && count) memcpy(dest, src, count)

void __tapki_vec_erase(void *_vec, size_t idx, size_t tsz)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (_TAPKI_UNLIKELY(vec->size <= idx))
        TapkiDieF("vector.erase: index(%zu) > size(%zu)", idx, vec->size);
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
    fflush(stderr);
    abort();
}

void TapkiDieF(const char *fmt, ...)
{
    TapkiArena* a = TapkiArenaCreate(1024);
    va_list vargs;
    va_start(vargs, fmt);
    TapkiDie(TapkiVF(a, fmt, vargs).data);
}

void TapkiVecClear(void *_vec)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    vec->size = 0;
}

static TapkiStr __tapkis_withn(TapkiArena* ar, size_t res) {
    TapkiStr str = {0};
    TapkiVecReserve(ar, &str, res);
    str.size = res;
    str.data[res] = 0;
    return str;
}

void __tapki_vec_append(TapkiArena *ar, void *_vec, void *data, size_t count, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    __tapki_vec_reserve(ar, vec, vec->size + count, tsz, al);
    _TAPKI_MEMSET(vec->data + vec->size * tsz, data, count * tsz);
    vec->size += count;
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
        void* out = target->data + target->size;
        _TAPKI_MEMSET(out, strs[i], lens[i]);
        target->size += lens[i];
    }
    target->data[target->size] = 0;
    return target;
}

TapkiStr __tapkis_stos(TapkiArena *ar, const char *s)
{
    size_t n = strlen(s);
    TapkiStr res = __tapkis_withn(ar, n);
    _TAPKI_MEMSET(res.data, s, n);
    return res;
}

TapkiStr __tapkis_pcopy(TapkiArena *ar, const TapkiStr *str)
{
    TapkiStr res = __tapkis_withn(ar, str->size);
    _TAPKI_MEMSET(res.data, str->data, str->size);
    return res;
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
    _TAPKI_MEMSET(res.data, target + from, diff);
    return res;
}

size_t TapkiStrFind(const char *target, const char *what, size_t offset)
{
    const char* found = strstr(target + offset, what);
    if (!found) return Tapki_npos;
    return (size_t)(found - target);
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

void TapkiStrErase(TapkiStr *target, size_t index)
{
    TapkiVecErase(target, index);
    target->data[target->size] = 0;
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
    if (_TAPKI_UNLIKELY(!next)) TapkiDie("arena.chunk.new");
    next->cap = cap;
    next->next = NULL;
    if (ar->current) ar->current->next = next;
    ar->current = next;
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

void *TapkiArenaAllocAligned(TapkiArena *arena, size_t size, size_t align)
{
    size_t tail = arena->ptr & (align - 1);
    size_t aligned = tail ? arena->ptr + align - tail : arena->ptr;
    size_t end = aligned + size;
    void* result;
    if (_TAPKI_UNLIKELY(end > arena->current->cap)) {
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

char *__tapki_vec_reserve(TapkiArena *ar, void *_vec, size_t count, size_t tsz, size_t al)
{
    if (tsz == 1)
        count++;
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (vec->cap <= count) {
        uintptr_t arenaEnd = (uintptr_t)(ar->current->buff + ar->ptr);
        uintptr_t vecEnd = (uintptr_t)(vec->data + vec->cap * tsz);
        size_t ncap = vec->cap * 2;
        vec->cap = (ncap < count ? count : ncap);
        uintptr_t vecNewEnd = (uintptr_t)(vec->data + vec->cap * tsz);
        uintptr_t arenaCap = (uintptr_t)(ar->current->buff + ar->current->cap);
        // If we can just grow arena (vector is at the end of it) we do not relocate
        if (arenaEnd == vecEnd && vecNewEnd < arenaCap) {
            ar->ptr += (uintptr_t)(vecNewEnd - vecEnd);
        } else {
            char* newData = (char*)TapkiArenaAllocAligned(ar, vec->cap * tsz, al);
            _TAPKI_MEMSET(newData, vec->data, vec->size * tsz);
            vec->data = newData;
        }
    }
    if (tsz == 1 && vec->data)
        vec->data[count - 1] = 0;
    return vec->data;
}

TapkiStr TapkiVF(TapkiArena *ar, const char *fmt, va_list list)
{
    va_list list2;
    va_copy(list2, list);
    size_t count = vsnprintf(NULL, 0, fmt, list);
    TapkiStr result = __tapkis_withn(ar, count);
    vsnprintf(result.data, result.cap, fmt, list2);
    va_end(list2);
    return result;
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
    _TAPKI_MEMSET(result.data, s, len);
    return result;
}

TapkiStr *TapkiStrMapAt(TapkiArena *ar, TapkiStrMap *map)
{

}

void TapkiStrMapErase(TapkiStrMap *map, const char *key)
{

}

void TapkiWriteFile(const char *file, const char *contents)
{
}

void TapkiAppendFile(const char *file, const char *contents)
{

}

TapkiCLI TapkiParseCLI(TapkiArena *ar, int argc, const char * const *argv)
{
    TapkiCLI res = {0};

    return res;
}

TapkiStr TapkiReadFile(TapkiArena *ar, const char *file)
{
    FILE* f = fopen(file, "rb");
    if (!f)
        TapkiDieF("Could not open for read: %s => %s\n", file, strerror(errno));
    if (fseek(f, 0, SEEK_END))
        TapkiDieF("Could not seek file: %s => %s\n", file, strerror(errno));
    TapkiStr str = __tapkis_withn(ar, ftell(f));
    fseek(f, 0, SEEK_SET);
    fread(str.data, str.size, 1, f);
    fclose(f);
    return str;
}


#undef _TAPKI_MEMSET

#ifdef __cplusplus
}
#endif
