#include "dynarray.h"

// TODO: use own allocator rather than malloc
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void arrFree(Array arr) {
    free(arr.data);
}

Array _arrNew(size_t itemSize) {
    void* data = malloc(ARRAY_INIT_CAP * itemSize);
    (void) itemSize;
    return (Array) {
#ifndef DYNARRAY_UNSAFE
        .itemSize = itemSize,
#endif
        .size = 0,
        .cap = ARRAY_INIT_CAP,
        .data = data,
    };
}

bool _arrGrow(Array* arr, size_t itemSize) {
#ifndef DYNARRAY_UNSAFE
    assert(itemSize == arr->itemSize);
#endif
    size_t newCap = arr->cap << 1; // assume this will never overflow
    void* newData = realloc(arr->data, itemSize * newCap);
    if (newData == NULL) {
        return false;
    }
    arr->cap = newCap;
    arr->data = newData;
    return true;
}

bool _arrPush(Array* arr, void* addr, size_t itemSize) {
#ifndef DYNARRAY_UNSAFE
    assert(itemSize == arr->itemSize);
#endif
    while (arr->size >= arr->cap - 1) {
        if (!_arrGrow(arr, itemSize)) {
            return false; // this shouldn't happen often
        }
    }
    memcpy(arr->data + (itemSize * arr->size++), addr, itemSize);
    return true;
}

bool _arrErase(Array* arr, size_t idx, size_t num, size_t itemSize) {
#ifndef DYNARRAY_UNSAFE
    assert(itemSize == arr->itemSize);
    if (idx+num > arr->size) {
        return false;
    }
#endif
    memmove(arr->data + itemSize * idx,
            arr->data + itemSize * (idx+num),
            (arr->size-idx-num) * itemSize);
    arr->size -= num;
    return true;
}

bool _arrIndex(Array* arr, void* addr, size_t* idx, CmpPtr cmp, size_t itemSize) {
#ifndef DYNARRAY_UNSAFE
    assert(itemSize == arr->itemSize);
#endif
    for (size_t i = 0; i < arr->size; ++i) {
        if (cmp(arr->data + itemSize * i, addr) == 0) {
            if (idx != NULL) {
                *idx = i;
            }
            return true;
        }
    }
    return false;
}

void* _arrGet(Array* arr, size_t idx, size_t itemSize) {
#ifndef DYNARRAY_UNSAFE
    assert(itemSize == arr->itemSize);
    if (idx >= arr->size) {
        return NULL;
    }
#endif
    return arr->data + itemSize * idx;
}
