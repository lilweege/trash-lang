// #define DYNARRAY_UNSAFE
#include "../src/dynarray.c"
#include <stdio.h>

typedef struct {
    int x;
    char y;
} Foo;

int cmpFoo(const void* a, const void* b) {
    Foo* x = (Foo*) a;
    Foo* y = (Foo*) b;
    if (x->x != y->x) return x->x - y->x;
    return x->y - y->y;
}

SPECIALIZE_DYNARRAY(Foo, cmpFoo)

void printFooArr(Array* arr) {
    for (size_t i = 0; i < arr->size; ++i) {
        Foo f = *arrGetFoo(arr, i);
        printf("%d, '%c'\n", f.x, f.y);
    }
}

void testDynarray() {
    Array arr = arrNewFoo();

    for (int i = 1; i <= 5; ++i) {
        arrPushFoo(&arr, (Foo){ i, (char)i + 'a' - 1 });
    }
    assert(arrEraseFoo(&arr, 1, 2));
    printFooArr(&arr);

    size_t idx;
    if (arrIndexFoo(&arr, (Foo){ 4, 'd' }, &idx)) {
        printf("idx=%zu\n", idx);
    }
    else {
        printf("Not found\n");
    }
    
    Foo f = (Foo){ 1, 'a' };
    for (size_t i = 0; i < 100; ++i) {
        arrPushFoo(&arr, f);
    }
    printf("size=%zu\n", arr.size);
    printf("cap=%zu\n", arr.cap);

    arrFree(arr);
}
