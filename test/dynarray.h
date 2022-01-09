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
        if (i > 0) printf(", ");
        printf("{%d, '%c'}", f.x, f.y);
    }
    putchar('\n');
}

void testDynarray() {
    puts("==== TEST DYNARRAY ====");
    size_t idx;
    Array arr = arrNewFoo();

    for (int i = 1; i <= 5; ++i) {
        arrPushFoo(&arr, (Foo){ i, (char)i + 'a' - 1 });
    }
    assert(arrEraseFoo(&arr, 1, 2));

    Array cp = arrCopyFoo(arr);
    arrPushFoo(&cp, (Foo){0});
    assert(arrIndexFoo(&cp, (Foo){ 5, 'e' }, &idx));
    arrEraseFoo(&cp, idx, 1);

    printFooArr(&arr);
    printFooArr(&cp);

    arrFree(cp);

    if (arrIndexFoo(&arr, (Foo){ 4, 'd' }, &idx)) {
        printf("idx=%zu\n", idx);
    }
    else {
        printf("Not found\n");
        assert(0);
    }
    assert(idx == 1);
    
    Foo f = (Foo){ 1, 'a' };
    for (size_t i = 0; i < 100; ++i) {
        arrPushFoo(&arr, f);
    }
    printf("size=%zu\n", arr.size);
    printf("cap=%zu\n", arr.cap);

    arrFree(arr);
    puts("==== DONE DYNARRAY ====");
}
