#include "tapki.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _TAPKI_MEMSET(dest, src, count) if (src && count) memcpy(dest, src, count)

_Thread_local __tpk_frames __tpk_gframes;

void __tpk_frame_start(__tpk_scope *scope, ...)
{
    va_start(scope->vargs, scope);
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
    va_end(scope->vargs);
}

void __tapki_vec_erase(void *_vec, size_t idx, size_t tsz)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (TAPKI_UNLIKELY(vec->size <= idx))
        TapkiDie("vector.erase: index(%zu) > size(%zu)", idx, vec->size);
    size_t tail = (--vec->size) - idx;
    char* dest = vec->d + idx * tsz;
    char* src = dest + tsz;
    if (tail) {
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
        TapkiStr msg = TapkiF(arena, "  in %-*s '%s'", longest, frame->s->loc, frame->s->func);
        TapkiStrAppend(arena, &result, msg.d);
        if (frame->s->fmt) {
            TapkiStrAppend(arena, &result, " => ");
            TapkiStr context = TapkiVF(arena, frame->s->fmt, frame->s->vargs);
            TapkiStrAppend(arena, &result, context.d);
        }
        TapkiVecAppend(arena, &result, '\n');
    }
    return result;
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
    fputs(TapkiTraceback(arena).d, stderr);
    fputc('\n', stderr);
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
    _TAPKI_MEMSET(vec->d + vec->size * tsz, data, count * tsz);
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
        _TAPKI_MEMSET(out, strs[i], lens[i]);
        target->size += lens[i];
    }
    if (target->d) {
        target->d[target->size] = 0;
    }
    return target;
}

TapkiStr __tapkis_stos(TapkiArena *ar, const char *s)
{
    size_t n = strlen(s);
    TapkiStr res = __tapkis_withn(ar, n);
    _TAPKI_MEMSET(res.d, s, n);
    return res;
}

TapkiStr __tapkis_pcopy(TapkiArena *ar, const TapkiStr *str)
{
    TapkiStr res = __tapkis_withn(ar, str->size);
    _TAPKI_MEMSET(res.d, str->d, str->size);
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
    _TAPKI_MEMSET(res.d, target + from, diff);
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
    char* src = vec->d + idx * tsz;
    char* dest = src + tsz;
    memmove(dest, src, tail * tsz);
    if (tsz == 1) {
        vec->d[vec->size] = 0;
    }
    return vec->d + idx * tsz;
}

void* __tapki_map_at(TapkiArena *ar, void* _map, const void* key, const __tpk_map_info *info)
{
    __TapkiVec* map = (__TapkiVec*)_map;
    char *end = map->d + map->size * info->pair_sizeof;
    char *it = info->lower_bound(map->d, end, key);
    if (it == end) {
        char* pushed = __tapki_vec_push(ar, map, info->pair_sizeof, info->pair_align);
        memcpy(pushed, key, info->key_sizeof);
        return pushed + info->value_offset;
    } else if (info->eq(it, key)) {
        return it + info->value_offset;
    } else {
        size_t insert_at = (it - map->d) / info->pair_sizeof;
        char* inserted = __tapki_vec_insert(ar, map, insert_at, info->pair_sizeof, info->pair_align);
        memcpy(inserted, key, info->key_sizeof);
        return inserted + info->value_offset;
    }
}

void *__tapki_map_find(void *_map, const void *key, const __tpk_map_info *info)
{
    __TapkiVec* map = (__TapkiVec*)_map;
    char *end = map->d + map->size * info->pair_sizeof;
    char *it = info->lower_bound(map->d, end, key);
    if (it != end && info->eq(it, key)) {
        return it + info->value_offset;
    } else {
        return NULL;
    }
}

bool __tapki_map_erase(void *_map, const void *key, const __tpk_map_info* info)
{
    __TapkiVec* map = _map;
    char *end = map->d + map->size * info->pair_sizeof;
    char *it = info->lower_bound(map->d, end, key);
    if (info->eq(it, key)) {
        size_t idx = (it - map->d) / info->pair_sizeof;
        __tapki_vec_erase(map, idx, info->pair_sizeof);
        return true;
    } else {
        return false;
    }
}

#define __TPK_SMAP_LESS(l, r) strcmp(l, r) < 0
#define __TPK_SMAP_EQ(l, r) strcmp(l, r) == 0

TapkiMapImplement(TapkiStrMap, __TPK_SMAP_LESS, __TPK_SMAP_EQ)

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
            _TAPKI_MEMSET(newData, vec->d, vec->size * tsz);
            vec->d = newData;
        }
        if (tsz == 1 && vec->d)
            vec->d[count - 1] = 0;
    }
    return vec->d;
}

TapkiStr TapkiVF(TapkiArena *ar, const char *fmt, va_list list)
{
    va_list list2;
    va_copy(list2, list);
    size_t count = vsnprintf(NULL, 0, fmt, list);
    TapkiStr result = __tapkis_withn(ar, count);
    vsnprintf(result.d, result.cap, fmt, list2);
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
    _TAPKI_MEMSET(result.d, s, len);
    return result;
}

void TapkiStrAppend(TapkiArena *ar, TapkiStr *target, const char *part)
{
    size_t len = strlen(part);
    TapkiVecReserve(ar, target, target->size + len);
    _TAPKI_MEMSET(target->d + target->size, part, len);
    target->size += len;
    if (target->d) {
        target->d[target->size] = 0;
    }
}

static FILE* __tpk_open(const char* file, const char* mode, const char* comment) {
    FILE* f = fopen(file, mode);
    if (!f) {
        TapkiDie("Could not open for %s: %s => %s\n", comment, file, strerror(errno));\
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
        TapkiDie("Could fully not convert to u64: %s", s);
    }
    return res;
}

int64_t TapkiToI64(const char *s)
{
    char* end;
    int64_t res = strtoll(s, &end, 10);
    if (TAPKI_UNLIKELY(*end != 0)) {
        TapkiDie("Could fully not convert to u64: %s", s);
    }
    return res;
}

uint32_t TapkiToU32(const char *s)
{
    uint64_t res = TapkiToI64(s);
    if (TAPKI_UNLIKELY(res > UINT32_MAX)) {
        TapkiDie("Integer is too big for u32: %s", s);
    }
    return (uint32_t)res;
}

int32_t TapkiToI32(const char *s)
{
    int64_t res = TapkiToI64(s);
    if (TAPKI_UNLIKELY(res > INT32_MAX)) {
        TapkiDie("Integer is too big for i32: %s", s);
    }
    return (int32_t)res;
}

TapkiCLIVarsResult TapkiCLI_ParseVars(TapkiArena *ar, TapkiCLI cli[], int argc, char** argv)
{
    TapkiCLIVarsResult res = {0};

    return res;
}


TapkiStr TapkiCLI_Help(TapkiArena *ar, TapkiCLI cli[])
{

}

TapkiStr TapkiCLI_Usage(TapkiArena *ar, TapkiCLI cli[], int argc, char **argv)
{
    TapkiStr result = TapkiS(ar, argv[0]);
    TapkiStrAppend(ar, &result, ": ");
}

int TapkiParseCLI(TapkiArena *ar, TapkiCLI cli[], int argc, char **argv)
{
    TapkiCLIVarsResult result = TapkiCLI_ParseVars(ar, cli, argc, argv);
    if (!result.ok) {
        fprintf(stderr, "%s\n", result.error.d);
        fprintf(stderr, "Usage: %s\n", TapkiCLI_Usage(ar, cli, argc, argv).d);
        fprintf(stderr, "%s\n", TapkiCLI_Help(ar, cli).d);
        return 1;
    }
    return 0;
}

TapkiStr TapkiReadFile(TapkiArena *ar, const char *file)
{
    FILE* f = __tpk_open(file, "rb", "read");
    if (fseek(f, 0, SEEK_END))
        TapkiDie("Could not seek file: %s => %s\n", file, strerror(errno));
    TapkiStr str = __tapkis_withn(ar, ftell(f));
    fseek(f, 0, SEEK_SET);
    fread(str.d, str.size, 1, f);
    fclose(f);
    return str;
}

#ifdef _WIN32
static char __tpk_sep = '\\'
#else
static char __tpk_sep = '/';
#endif

TapkiStr __tpk_path_join(TapkiArena *ar, const char **parts, size_t count)
{
    TapkiStr res = {0};
    for (size_t i = 0; i < count; ++i) {
        const char* part = parts[i];
        if (i) {
            TapkiVecAppend(ar, &res, __tpk_sep);
        }
        TapkiStrAppend(ar, &res, part);
    }
    return res;
}


#undef _TAPKI_MEMSET

#ifdef __cplusplus
}
#endif
