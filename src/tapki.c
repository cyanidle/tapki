#include "tapki.h"

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

#define _TAPKI_MEMSET(dest, src, count) if (src && count) memcpy(dest, src, count)

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

/* By liw. */
static const char *__rev_strstr(const char *haystack, const char *needle) {
    if (*needle == '\0')
        return (char *) haystack;
    char *result = NULL;
    for (;;) {
        char *p = strstr(haystack, needle);
        if (p == NULL)
            break;
        result = p;
        haystack = p + 1;
    }
    return result;
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

TapkiMapImplement(TapkiStrMap, TAPKI_STRING_LESS, TAPKI_STRING_EQ)

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
        if (count < cap) {
            if (count) memcpy(str->d + was, buff, count + 1);
        } else {
            TapkiVecResize(ar, str, str->size + count);
            TapkiStr result = __tapkis_withn(ar, count);
            vsnprintf(str->d + was, str->cap, fmt, list2);
        }
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
    _TAPKI_MEMSET(result.d, s, len);
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
    int hits;
} __tpk_cli_priv;

typedef struct {
    __tpk_cli_priv* info;
    const char* alias;
} __tpk_cli_spec;

TapkiMapDeclare(__tpk_cli_named, char*, __tpk_cli_spec);
TapkiMapImplement(__tpk_cli_named, TAPKI_STRING_LESS, TAPKI_STRING_EQ);

typedef struct {
    __tpk_cli_named named;
    TapkiVec(__tpk_cli_spec) pos;
    TapkiVec(__tpk_cli_priv) _storage;
    bool need_help;
    TapkiCLI tpk_help_opt;
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
    while(curr->names) {
        count++;
        curr++;
    }
    TapkiVecResize(ar, &ctx->_storage, count);
    curr = cli;
}

static void __tpk_cli_reset(TapkiArena *ar, __tpk_cli_context* ctx)
{
    TapkiVecForEach(&ctx->_storage, it) {
        *it = (__tpk_cli_priv){it->orig};
    }
}

static void __tpk_cli_init(TapkiArena *ar, __tpk_cli_context* ctx, const TapkiCLI _source_spec[])
{
    ctx->need_help = false;
    ctx->tpk_help_opt = (TapkiCLI){"-h,--help", &ctx->need_help, .flag=true, .help="show this help message and exit"};
    {
        size_t count = 0;
        const TapkiCLI* curr = _source_spec;
        while(curr->names) {
            count++;
            curr++;
        }
        TapkiVecReserve(ar, &ctx->_storage, count + 1);
        ctx->_storage.size = count;
        curr = _source_spec;
        TapkiVecForEach(&ctx->_storage, it) {
            *it = (__tpk_cli_priv){curr++};
        }
        TapkiVecAppend(ar, &ctx->_storage, (__tpk_cli_priv){&ctx->tpk_help_opt});
    }
    TapkiArena* temp = TapkiArenaCreate(256);
    TapkiVecForEach(&ctx->_storage, it) {
        FrameF("Parse CLI argument spec (#%zu): %s", it->orig - _source_spec, it->orig->names) {
            for (__tpk_cli_priv* check = it + 1; check != ctx->_storage.d + ctx->_storage.size; ++check) {
                if (check->orig->data == it->orig->data) {
                    Die("Conflict: output &pointer already used in argument: %s", check->orig->names);
                }
            }
            TapkiAssert(it->orig->data != NULL && "Pointer to variable missing");
            char *saveptr = NULL;
            char *names = TapkiS(ar, it->orig->names).d;
            size_t dashes;
            char* alias;
            bool was_pos = false;
            bool was_named = false;
            while((alias = strtok_r(saveptr ? NULL : names, ",", &saveptr))) {
                alias = (char*)__tpk_cli_dashes(alias, &dashes);
                if (dashes) {
                    if (it->orig->flag && it->orig->required) {
                        Die("'flag' arguments cannot be also 'required'");
                    }
                    was_named = true;
                    *__tpk_cli_named_At(ar, &ctx->named, alias) = (__tpk_cli_spec){it, alias};
                } else {
                    was_pos = true;
                    if (it->orig->flag) {
                        Die("Positional arguments cannot be flags");
                    }
                    if (ctx->pos.size && TapkiVecAt(&ctx->pos, ctx->pos.size - 1)->info->orig->many) {
                        Die("Positional argument cannot follow another with 'many' flag (it will consume all)");
                    }
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
    for(int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (named) {
        parse_named:
            if (!__tpk_cli_parse(ar, named, i, arg, &result.error)) {
                return result;
            }
            named = NULL;
            continue;
        }
        size_t dashes;
        arg = __tpk_cli_dashes((char*)arg, &dashes);
        if (dashes == 0) {
            if (pos == pos_end) {
                result.error = TapkiF(ar, "unexpected positional argument (#%d): %s", i, arg);
                return result;
            }
            if (!__tpk_cli_parse(ar, pos, i, arg, &result.error)) {
                return result;
            }
            if (!pos->info->orig->many) {
                pos++;
            }
        } else if (dashes < 3) {
            const char* eq = strchr(arg, '=');
            char* lookup = (char*)arg;
            if (eq) {
                arg = eq + 1;
                lookup = TapkiS(ar, arg).d;
                lookup[eq - arg] = 0;
            }
            named = __tpk_cli_named_Find(&ctx->named, lookup);
            if (!named) {
                result.error = TapkiF(ar, "unknown argument (#%d): %s", i, lookup);
                return result;
            }
            if (named->info->hits && !named->info->orig->many) {
                result.error = TapkiF(ar, "argument (#%d): %s: more than one value provided", i, lookup);
                return result;
            }
            if (named->info->orig->flag) {
                *(bool*)named->info->orig->data = true;
                if (TAPKI_UNLIKELY(eq)) {
                    result.error = TapkiF(ar, "argument (#%d): %s: cannot assign with '=' to 'flag' named argument", i, lookup);
                    return result;
                }
                named = NULL;
            }
            if (eq) {
                goto parse_named;
            }
        } else {
            result.error = TapkiF(ar, "unexpected amount of '-' in argument (#%d): %s", i, arg);
            return result;
        }
    }
    VecForEach(&ctx->_storage, spec) {
        if (spec->orig->required && !spec->hits) {
            result.error = TapkiF(ar, "missing argument: %s", spec->orig->names);
            return result;
        }
    }
    result.ok = true;
    result.need_help = ctx->need_help;
    return result;
}

/*
usage: test.py [-h] [--long LONG] pos

positional arguments:
  pos                   Pos arg

options:
  -h, --help            show this help message and exit
  --long LONG, -l LONG  Named
*/

static TapkiStr __TapkiCLI_Usage(TapkiArena *ar, __tpk_cli_context* ctx, int argc, char **argv)
{
    size_t offset = TapkiStrRevFind(argv[0], __tpk_sep);
    TapkiStr result = TapkiS(ar, argv[0] + (offset == Tapki_npos ? 0 : offset + 1));
    TapkiStrAppend(ar, &result, ": ");
    TapkiVecForEach(&ctx->named, named) {
        const TapkiCLI* cli = named->value.info->orig;
        if (named->value.info->hits++) continue;
        TapkiStrAppendF(ar, &result, " [%s]", named->value.alias);
        //  TODO:
    }
    TapkiVecForEach(&ctx->pos, pos) {
        const TapkiCLI* cli = pos->info->orig;
        if (pos->info->hits++) continue;
        TapkiStrAppend(ar, &result, " ", pos->alias);
        if (pos->info->orig->many) {
            TapkiStrAppend(ar, &result, "...");
        }
    }
    return result;
}

static TapkiStr __TapkiCLI_Help(TapkiArena *ar, __tpk_cli_context* ctx)
{
    TapkiStr result = {0};
    //todo:
    return result;
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
    }
    return 0;
}

TapkiStr TapkiReadFile(TapkiArena *ar, const char *file)
{
    FILE* f = __tpk_open(file, "rb", "read");
    if (fseek(f, 0, SEEK_END))
        TapkiDie("Could not seek file: %s => [Errno: %d] %s\n", file, errno, strerror(errno));
    TapkiStr str = __tapkis_withn(ar, ftell(f));
    fseek(f, 0, SEEK_SET);
    if (fread(str.d, str.size, 1, f))
        TapkiDie("Could not read file: %s => [Errno: %d] %s\n", file, errno, strerror(errno));
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


#undef _TAPKI_MEMSET

#ifdef __cplusplus
}
#endif
