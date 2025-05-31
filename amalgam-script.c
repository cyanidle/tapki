#include "src/tapki.c"

#define ASSERT(...) Frame() { if (!(__VA_ARGS__)) Die("Test failed: " #__VA_ARGS__); } (void)0

void Test_Maps(Arena* arena) {
    StrMap map = {0};
    *StrMap_At(&map, "Kek") = S("Lol");
    *StrMap_At(&map, "1") = S("1");
    *StrMap_At(&map, "2") = S("2");
    *StrMap_At(&map, "3") = S("3");
    ASSERT(map.size == 4);
    StrMap_Erase(&map, "2");
    ASSERT(map.size == 3);
    ASSERT(strcmp(StrMap_Find(&map, "3")->d, "3") == 0);
    ASSERT(strcmp(StrMap_Find(&map, "Kek")->d, "Lol") == 0);
    ASSERT(!StrMap_Find(&map, "2"));
    Str* kek = StrMap_At(&map, "Kek");
    StrAppend(kek, "Kek");
    ASSERT(strcmp(StrMap_Find(&map, "Kek")->d, "LolKek") == 0);
}

void test() {
    Frame() {
        Arena* arena = ArenaCreate(1024 * 20);
        FrameF("Maps") {
            Test_Maps(arena);
        }
        ArenaFree(arena);
    }
}

int main(int argc, char** argv) {
    test();
    Arena* arena = ArenaCreate(1024 * 20);
    Str base_dir = S(".");
    CLI cli[] = {
        {"dir", &base_dir, .help = "Root of tapki repository"},
        {0},
    };
    int ret = ParseCLI(cli, argc, argv);
    if (ret != 0) {
        goto end;
    }
    Str header = ReadFile(PathJoin(base_dir.d, "src/tapki.h").d);
    Str source = ReadFile(PathJoin(base_dir.d, "src/tapki.c").d);
    const char* output = PathJoin(base_dir.d, "tapki.h").d;

    size_t firstLine = StrFind(source.d, "\n", 0);
    WriteFile(output, F("%s\n\n#ifndef TAPKI_IMPLEMENTATION\n%s#endif //TAPKI_IMPLEMENTATION\n", header.d, source.d + firstLine).d);
end:
    ArenaFree(arena);
    return ret;
}
