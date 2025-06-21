#define TAPKI_IMPLEMENTATION
#include "tapki.h"

#define ASSERT(...) Frame() { if (!(__VA_ARGS__)) Die("Test failed: " #__VA_ARGS__); } (void)0

void Test_Maps(Arena* arena) {
    StrMap map = {0};
    *StrMap_At(&map, "Kek") = S("Lol");
    *StrMap_At(&map, "1") = S("1");
    *StrMap_At(&map, "2") = S("2");
    *StrMap_At(&map, "3") = S("3");
    ASSERT(map.size == 4);
    TapkiMapForEach(&map, it) {

    }
    StrMap_Erase(&map, "2");
    ASSERT(map.size == 3);
    ASSERT(strcmp(StrMap_Find(&map, "3")->d, "3") == 0);
    ASSERT(strcmp(StrMap_Find(&map, "Kek")->d, "Lol") == 0);
    ASSERT(!StrMap_Find(&map, "2"));
    Str* kek = StrMap_At(&map, "Kek");
    StrAppend(kek, "Kek");
    ASSERT(strcmp(StrMap_Find(&map, "Kek")->d, "LolKek") == 0);
}

void Test() {
    Frame() {
        Arena* arena = ArenaCreate(1024 * 20);
        FrameF("Maps") {
            Test_Maps(arena);
        }
        ArenaFree(arena);
    }
}

int main(int argc, char** argv) {
    Arena* arena = ArenaCreate(1024 * 20);
    Str base_dir = S(".");
    Str test = S(".");
    Str file = S(".");
    CLI cli[] = {
        {"dir", &base_dir, .help = "Example positional"},
        {"--test,-t", &test, .help = "Example named"},
        {"--file,-f", &file, .metavar = "FILE", .help = "Example named"},
        {0},
    };
    Test();
    int ret = ParseCLI(cli, argc, argv);
    if (ret != 0) {
        goto end;
    }
end:
    ArenaFree(arena);
    return ret;
}
