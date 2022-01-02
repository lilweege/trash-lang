#include "stringView.h"

#include <string.h>

StringView svNew(char* data, size_t size) {
    return (StringView) {
        .data = data,
        .size = size,
    };
}

StringView svFromCStr(char *cstr) {
    return svNew(cstr, strlen(cstr));
}

StringView svLeftTrim(StringView *sv, size_t* outLineNo, size_t* outColNo) {
    size_t x = 0;
    while (x < sv->size) {
        char c = sv->data[x];
        if (!(c == ' ' || c == '\r' || c == '\n' || c == '\t')) {
            break;
        }
        if (outLineNo != NULL) {
            if (c == '\n') {
                ++*outLineNo;
            }
        }
        if (outColNo != NULL) {
            if (c == '\n') {
                *outColNo = 0;
            }
            else {
                ++*outColNo;
            }
        }
        ++x;
    }
    return svLeftChop(sv, x);
}

StringView svLeftChop(StringView *sv, size_t n) {
    if (n > sv->size) {
        n = sv->size;
    }
    StringView chopped = svNew(sv->data, n);
    sv->data += n;
    sv->size -= n;
    return chopped;
}

StringView svLeftChopWhile(StringView *sv, bool (*predicate)(char c)) {
    size_t x = 0;
    while (x < sv->size && predicate(sv->data[x])) {
        ++x;
    }
    return svLeftChop(sv, x);
}

StringView svSubstring(StringView sv, size_t beginIdx, size_t endIdx) {
    return svNew(sv.data + beginIdx, endIdx - beginIdx);
}

int64_t svFirstIndexOfChar(StringView sv, char c) {
    for (size_t x = 0; x < sv.size; ++x) {
        if (sv.data[x] == c) {
            return x;
        }
    }
    return -1;
}

int64_t svFirstIndexOf(StringView src, StringView str) {
    if (str.size > src.size) {
        return -1;
    }
    // // TODO:
    // // strstr is incorrect, ideally something like strnstr would work
    // // Unfortunately strnstr is not standard, so I may implement
    // // my own substr match at some point. IIRC some strstr
    // // implementations use a modified version of the boyer-moore
    // // char* res = strstr(src.data, str.data);
    // if (res == NULL)
    //     return -1;
    // return res - src.data;
    
    // FIXME: naive implementation
    for (size_t i = 0; i < src.size - str.size; ++i) {
        if (memcmp(src.data + i, str.data, str.size) == 0) {
            return i;
        }
    }
    return -1;
}

int svCmp(StringView a, StringView b) {
    if (a.size != b.size)
        return a.size - b.size;
    // NOTE: memcmp works exactly the same here but doesn't stop at null
    // characters. In regular use there is no difference between the two
    return memcmp(a.data, b.data, a.size);
}

char svPeek(StringView sv, size_t n) {
    return n < sv.size ? sv.data[n] : -1;
}
