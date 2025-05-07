#include "src/tapki.c"

int main(int argc, char** argv) {
    Arena* arena = ArenaCreate(1024 * 20);
    Str base_dir = S(".");
    CLI cli[] = {
        {"dir", &base_dir},
        {0},
    };
    int ret = ParseCLI(cli, argc, argv);
    if (ret != 0) {
        goto end;
    }
    Str header = ReadFile(C(PathJoin(C(base_dir), "src/tapki.h")));
    Str source = ReadFile(C(PathJoin(C(base_dir), "src/tapki.c")));
end:
    ArenaFree(arena);
    return ret;
}
