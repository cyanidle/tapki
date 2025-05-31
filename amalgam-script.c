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
        Test_Maps(arena);
        ArenaFree(arena);
    }
}

int main(int argc, char** argv) {
    Arena* arena = ArenaCreate(1024 * 20);
    test();
    Str base_dir = S(".");
    bool test = false;
    Str kek = {0};
    IntVec vals = {0};
    CLI cli[] = {
        {"dir", &base_dir, .help = "help: aaaaaaaaaaaaaaaaa"},
        {"--test", &test, .flag = true, .help = "help: bbbbbbbbbbbb"},
        {"--kek", &kek, .help = "help: ccccccccccccc"},
        {"--val", &vals, .int64 = true, .many = true, .help = "help: dddddddddd"},
        {"renamed_prog", .program = true, .help = "Hello, program help!"},
        {0},
    };
    int ret = ParseCLI(cli, argc, argv);
    if (ret != 0) {
        goto end;
    }
    Str header = ReadFile(PathJoin(base_dir.d, "src/tapki.h").d);
    Str source = ReadFile(PathJoin(base_dir.d, "src/tapki.c").d);
end:
    ArenaFree(arena);
    return ret;
}
