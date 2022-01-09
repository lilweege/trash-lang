#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ARRAY_INIT_CAP 8

typedef struct {
#ifndef DYNARRAY_UNSAFE
    size_t itemSize;
#endif
    size_t size;
    size_t cap;
    uint8_t* data;
} Array;

#define SPECIALIZE_DYNARRAY(type, cmp) SPECIALIZE_DYNARRAY_NAMED(type, type, cmp)
#define SPECIALIZE_DYNARRAY_NAMED(name, type, cmp) \
Array arrNew##name()                                     { return _arrNew(sizeof(type), ARRAY_INIT_CAP);         } \
Array arrCopy##name(Array arr)                           { return _arrCopy(arr, sizeof(type));                   } \
bool arrGrow##name(Array* arr)                           { return _arrGrow(arr, sizeof(type));                   } \
bool arrPush##name(Array* arr, type item)                { return _arrPush(arr, &item, sizeof(type));            } \
bool arrErase##name(Array* arr, size_t idx, size_t num)  { return _arrErase(arr, idx, num, sizeof(type));        } \
bool arrIndex##name(Array* arr, type item, size_t* idx)  { return _arrIndex(arr, &item, idx, cmp, sizeof(type)); } \
type* arrGet##name(Array* arr, size_t idx)               { return (type*) _arrGet(arr, idx, sizeof(type));       }


// TODO: array shrink
typedef int (*CmpPtr)(const void*, const void*);

void arrFree(Array arr);
Array _arrNew(size_t itemSize, size_t initCap);
Array _arrCopy(Array arr, size_t itemSize);
bool _arrGrow(Array* arr, size_t itemSize);
bool _arrPush(Array* arr, void* addr, size_t itemSize);
bool _arrErase(Array* arr, size_t idx, size_t num, size_t itemSize);
bool _arrIndex(Array* arr, void* addr, size_t* idx, CmpPtr cmp, size_t itemSize);
void* _arrGet(Array* arr, size_t idx, size_t itemSize);

#endif // DYNARRAY_H
