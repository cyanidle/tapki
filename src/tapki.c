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

void __tpk_frame_start(const char* loc, const char* func, const __tpk_scope* scope)
{
    struct __tpk_frames* frames = &__tpk_gframes;
    if (TAPKI_UNLIKELY(!frames->arena)) {
        frames->arena = TapkiArenaCreate(1024);
    }
    __tpk_frame* fr = TapkiVecPush(frames->arena, &frames->frames);
    fr->loc = loc;
    fr->func = func;
    fr->scope = scope;
}

void __tpk_frame_end()
{
    struct __tpk_frames* frames = &__tpk_gframes;
    TapkiVecPop(&frames->frames);
}

void __tapki_vec_erase(void *_vec, size_t idx, size_t tsz)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (TAPKI_UNLIKELY(vec->size <= idx))
        TapkiDie("vector.erase: index(%zu) > size(%zu)", idx, vec->size);
    size_t tail = (--vec->size) - idx;
    if (tail) {
        memmove(vec->data + idx * tsz, vec->data + (idx + 1) * tsz, tail * tsz);
    }
    if (tsz == 1 && vec->data)
        vec->data[vec->size] = 0;
}

TapkiStr TapkiTraceback(TapkiArena *arena)
{
    TapkiStr result = TapkiS(arena, "Traceback (most recent on top):\n");
    int longest = 0;
    TapkiFramesIter(frame) {
        int len = strlen(frame->loc);
        if (len > longest) longest = len;
    }
    TapkiFramesIter(frame) {
        TapkiStr msg = TapkiF(arena, "  in %-*s '%s'", longest, frame->loc, frame->func);
        TapkiStrAppend(arena, &result, C(msg));
        // todo: print context
        TapkiVecAppend(arena, &result, '\n');
    }
    return result;
}

static bool __tpk_inside_die = false;
void TapkiDie(const char *fmt, ...)
{
    if (__tpk_inside_die) {
        fputs("Recursive Die()! Out of memory?\n", stderr);
        fflush(stderr);
        exit(1);
    }
    __tpk_inside_die = true;
    fputs("Fatal Error: ", stderr);
    { //user msg
        va_list vargs;
        va_start(vargs, fmt);
        vfprintf(stderr, fmt, vargs);
        va_end(vargs);
        fputc('\n', stderr);
    }
    TapkiArena* arena = TapkiArenaCreate(1024);
    fputs(TapkiC(TapkiTraceback(arena)), stderr);
    fputc('\n', stderr);
    fflush(stderr);
    exit(1);
}

void TapkiVecClear(void *_vec)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    vec->size = 0;
    if (vec->data) vec->data[0] = 0;
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
    __tapki_vec_reserve(ar, vec, vec->size + 1, tsz, al);
    size_t tail = vec->size++ - idx;
    char* src = vec->data + idx * tsz;
    char* dest = src + tsz;
    memmove(dest, vec->data + idx * tsz, tail * tsz);
    if (tsz == 1) {
        vec->data[vec->size] = 0;
    }
    return vec->data + idx * tsz;
}

char *__tapki_vec_reserve(TapkiArena *ar, void *_vec, size_t count, size_t tsz, size_t al)
{
    __TapkiVec* vec = (__TapkiVec*)_vec;
    if (tsz == 1)
        count++;
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
        if (tsz == 1 && vec->data)
            vec->data[count - 1] = 0;
    }
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

void TapkiStrAppend(TapkiArena *ar, TapkiStr *target, const char *part)
{
    size_t len = strlen(part);
    TapkiVecReserve(ar, target, target->size + len);
    _TAPKI_MEMSET(target->data + target->size, part, len);
}

static TapkiStrPair* __tpk_lower_bound(TapkiStrPair *begin, TapkiStrPair* end, const char* key) {
    TapkiStrPair * it;
    size_t count, step;
    count = end - begin;
    while (count > 0) {
        it = begin;
        step = count / 2;
        it += step;
        if (strcmp(it->key.data, key) < 0) {
            begin = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return begin;
}

TapkiStr *TapkiStrMapAt(TapkiArena *ar, TapkiStrMap *map, const char* key)
{
    TapkiStrPair *end = map->data + map->size;
    TapkiStrPair *it = __tpk_lower_bound(map->data, end, key);
    if (it == end) {
        TapkiStrPair* pushed = TapkiVecPush(ar, map);
        *(TapkiStr*)&pushed->key = TapkiS(ar, key);
        return &pushed->value;
    } else if (strcmp(it->key.data, key) == 0) {
        return &it->value;
    } else {
        TapkiStrPair* inserted = TapkiVecInsert(ar, map, it - map->data);
        *(TapkiStr*)&inserted->key = TapkiS(ar, key);
        return &inserted->value;
    }
}

void TapkiStrMapErase(TapkiStrMap *map, const char *key)
{
    TapkiStrPair *it = __tpk_lower_bound(map->data, map->data + map->size, key);
    if (strcmp(it->key.data, key) == 0) {
        size_t idx = it - map->data;
        TapkiVecErase(map, idx);
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
        fprintf(stderr, "%s\n", C(result.error));
        fprintf(stderr, "Usage: %s\n", C(TapkiCLI_Usage(ar, cli, argc, argv)));
        fprintf(stderr, "%s\n", C(TapkiCLI_Help(ar, cli)));
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
    fread(str.data, str.size, 1, f);
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
