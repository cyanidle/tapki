#include "src/tapki.c"

int main() {
    Arena* a = ArenaCreate(1024 * 20);
    Str kek = S(a, "Test string\n");
    //*VecInsert(a, &kek, 0) = '!';
    TapkiVecForEach(&kek, ch) {
        putc(*ch, stdout);
    }
    Str header = ReadFile(a, "./src/tapki.h");
    ArenaFree(a);
}
