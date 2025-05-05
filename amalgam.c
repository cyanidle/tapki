#include "src/tapki.c"

int main() {
    Arena* a = ArenaCreate(1024 * 20);
    Str header = ReadFile(a, "./src/tapki.h");
    ArenaFree(a);
}
