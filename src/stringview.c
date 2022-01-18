#include "stringview.h"

#include <string.h>
#include <ctype.h> // isalnum, isdigit
#include <inttypes.h> // strtoimax, strtoumax
#include <stdlib.h> // strtod


bool isIdentifier(char c) { return isalnum(c) || c == '_'; }
bool isNumeric(char c) { return isdigit(c); }

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

bool svToCStr(StringView sv, char* outBuf, size_t* outEnd) {
    size_t escaped = 0;
    size_t sz = sv.size;
    size_t i;
    for (i = 0; i < sz; ++i) {
        char ch = sv.data[i];
        if (ch == '\\') {
            ++escaped;
            if (++i >= sz) {
                return false;
            }
            char esc = sv.data[i];
            switch (esc) {
                case '"': ch = '"'; break;
                case 'n': ch = '\n'; break;
                case 't': ch = '\t'; break;
                case '\\': ch = '\\'; break;
                default: return false;
            }
        }
        outBuf[i-escaped] = ch;
    }
    outBuf[sz-escaped] = 0;
    if (outEnd != NULL) {
        *outEnd = sz-escaped;
    }
    return true;
}

// TODO: make these not suck
// unfortunately many implementations rely on null terminated strings
// there may be an easier way to do this but I am tired
#define SV_I64_MAX 20
#define SV_U64_MAX 19
#define SV_F64_MAX 24
int64_t svParseI64(StringView sv) {
    if (sv.size == 0 || sv.size > SV_I64_MAX) {
        return 0;
    }
    char buf[SV_I64_MAX+1];
    memcpy(buf, sv.data, sv.size);
    buf[sv.size] = 0;
    return strtoimax(buf, NULL, 10);
}

uint64_t svParseU64(StringView sv) {
    // NOTE: leading negative sign is not handled
    if (sv.size == 0 || sv.size > SV_U64_MAX) {
        return 0;
    }
    char buf[SV_U64_MAX+1];
    memcpy(buf, sv.data, sv.size);
    buf[sv.size] = 0;
    return strtoumax(buf, NULL, 10);
}

double svParseF64(StringView sv) {
    if (sv.size == 0 || sv.size > SV_F64_MAX) {
        return 0;
    }
    char buf[SV_F64_MAX+1];
    memcpy(buf, sv.data, sv.size);
    buf[sv.size] = 0;
    return strtod(buf, NULL);
}


