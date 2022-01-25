#ifndef STRINGVIEW_H
#define STRINGVIEW_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    const char* data;
} StringView;

#define SVNULL svNew(NULL, 0)
#define SV_FMT "%.*s"
#define SV_ARG(sv) ((int)(sv).size), ((sv).data)

bool isIdentifier(char c);
bool isNumeric(char c);

StringView svNew(size_t size, const char* data);
StringView svFromCStr(const char* cstr);
StringView svLeftTrim(StringView* sv, size_t* outLineNo, size_t* outColNo);
StringView svLeftChop(StringView* sv, size_t n);
StringView svLeftChopWhile(StringView* sv, bool (*predicate)(char c));
StringView svSubstring(StringView sv, size_t beginIdx, size_t endIdx);
bool svFirstIndexOfChar(StringView sv, char c, size_t* outIdx);
bool svLastIndexOfChar(StringView sv, char c, size_t* outIdx);
bool svFirstIndexOf(StringView src, StringView str, size_t* outIdx);
int svCmp(StringView a, StringView b);
char svPeek(StringView sv, size_t n);
uint32_t svHash(StringView sv);
bool svToCStr(StringView sv, char* outBuf, size_t* outEnd);
int64_t svParseI64(StringView sv);
uint64_t svParseU64(StringView sv);
double svParseF64(StringView sv);

// TODO: write some tests for sv operations

#endif // STRINGVIEW_H
