#include "tapki.c"

int main() {
    Arena* a = ArenaCreate(1024);
    Str s = F(a, "123: Kek: %d", 123);
    StrVec result = StrSplit(a, s.data, "2");
    for (int i = 0; i < 2000; ++i) {
        VecAppend(a, &result, s, s, s, s);
    }
    ArenaFree(a);
}
