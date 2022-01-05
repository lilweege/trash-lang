#include "stringview.h"

#include <string.h>

StringView svNew(size_t size, const char* data) {
    return (StringView) {
        .size = size,
        .data = data,
    };
}

StringView svFromCStr(const char* cstr) {
    return svNew(strlen(cstr), cstr);
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

StringView svLeftChop(StringView* sv, size_t n) {
    if (n > sv->size) {
        n = sv->size;
    }
    StringView chopped = svNew(n, sv->data);
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
    return svNew(endIdx - beginIdx, sv.data + beginIdx);
}

bool svFirstIndexOfChar(StringView sv, char c, size_t* outIdx) {
    for (size_t i = 0; i < sv.size; ++i) {
        if (sv.data[i] == c) {
            if (outIdx != NULL) {
                *outIdx = i;
            }
            return true;
        }
    }
    return false;
}

bool svFirstIndexOf(StringView src, StringView str, size_t* outIdx) {
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
            if (outIdx != NULL) {
                *outIdx = i;
            }
            return true;
        }
    }
    return false;
}

int svCmp(StringView a, StringView b) {
    if (a.size != b.size)
        return (int)(a.size - b.size);
    // NOTE: memcmp works exactly the same here but doesn't stop at null
    // characters. In regular use there is no difference between the two
    return memcmp(a.data, b.data, a.size);
}

char svPeek(StringView sv, size_t n) {
    return n < sv.size ? sv.data[n] : -1;
}

#define FNV1_32_INIT ((uint32_t) 0x811c9dc5)
#define FNV_32_PRIME ((uint32_t) 0x01000193)
uint32_t svHash(StringView sv) {
    uint32_t hash = FNV1_32_INIT;
    for (size_t i = 0; i < sv.size; ++i) {
	    hash ^= sv.data[i];
	    hash *= FNV_32_PRIME;
    }
    return hash;
}
