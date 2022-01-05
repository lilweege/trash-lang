#ifndef _STRINGVIEW_H
#define _STRINGVIEW_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char* data;
    size_t size;
} StringView;

#define SVNULL svNew(NULL, 0)
#define SV_FMT "%.*s"
#define SV_ARG(sv) ((int)(sv).size), ((sv).data)

StringView svNew(char *data, size_t size);
StringView svFromCStr(char *cstr);
StringView svLeftTrim(StringView* sv, size_t* outLineNo, size_t* outColNo);
StringView svLeftChop(StringView* sv, size_t n);
StringView svLeftChopWhile(StringView *sv, bool (*predicate)(char c));
StringView svSubstring(StringView sv, size_t beginIdx, size_t endIdx);
// indexOf will return index or -1 if not found
int64_t svFirstIndexOfChar(StringView sv, char c);
int64_t svFirstIndexOf(StringView src, StringView str);
int svCmp(StringView a, StringView b);
char svPeek(StringView sv, size_t n);
uint32_t svHash(StringView sv);

// TODO: write some tests for sv operations

#endif // _STRINGVIEW_H
