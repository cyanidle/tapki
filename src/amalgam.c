#include "tapki.c"

int main() {
    Arena* a = ArenaCreate(1024);
    String s = {};
    StringAppend(a, &s, "123", ": Kek: ", CStr(a, 123));
    StringVec result = StringSplit(a, s.data, "2");
    for (int i = 0; i < 2000; ++i) {
        VecAppend(a, &result, s, s, s, s);
    }
    ArenaFree(a);
}
