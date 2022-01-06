#include "../src/stringview.c"
#include "../src/hashmap.c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int randint(int a, int b) { // [a, b]
	return rand() % (a - b - 1) + a;
}

Key newRandomKey(size_t size) {
    char* data = (char*) malloc(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = (char) randint('!', '~');
    }
    return (Key) {
        .size = size,
        .data = data,
    };
}

void freeKey(Key sv) {
    free((void*) sv.data);
}

void hmDump(HashMap* hm) {
    for (size_t i = 0; i < hm->numBuckets; ++i) {
        if (hm->buckets[i].size > 0) {
            for (size_t j = 0; j < hm->buckets[i].size; ++j) {
                printf(SV_FMT"\n", SV_ARG((*arrGetKV(&hm->buckets[i], j)).key));
            }
            printf("\n");
        }
    }
}

#define NUM_KEYS (1<<15)
#define MIN_STRLEN 1
#define MAX_STRLEN 255
Key keys[NUM_KEYS];

void testHashmap() {
    puts("==== TEST HASHMAP ====");

    srand((int)time(NULL));
    HashMap hm = hmNew(16381);
    // HashMap hm = hmNew(101);

    for (size_t i = 0; i < NUM_KEYS; ++i) {
        do {
            keys[i] = newRandomKey(randint(MIN_STRLEN, MAX_STRLEN));
        } while (hmGet(&hm, keys[i]) != NULL);
        assert(hmPut(&hm, (Pair) { .key = keys[i], rand() }));
    }

    for (size_t i = 0; i < NUM_KEYS; ++i) {
        Pair p = (Pair) { .key = keys[i], rand() };
        Hint hnt = hmFind(&hm, p);
        assert(hnt.found);
        if (hmHintGet(&hm, hnt) == NULL) {
            assert(hmHintPut(&hm, hnt, p));
        }
    }

    // hmDump(&hm);

    for (size_t i = 0; i < NUM_KEYS; ++i) {
        assert(hmRemove(&hm, keys[i]));
    }

    size_t capMax = 0;
    for (size_t i = 0; i < hm.numBuckets; ++i) {
        assert(hm.buckets[i].size == 0);
        if (capMax < hm.buckets[i].cap)
            capMax = hm.buckets[i].cap;
    }

    printf("%zu\n", capMax);

    assert(hmPut(&hm,  (Pair) { .key = svFromCStr("asd") }));
    assert(hmPut(&hm,  (Pair) { .key = svFromCStr("sdf") }));
    assert(!hmPut(&hm, (Pair) { .key = svFromCStr("asd") }));

    for (size_t i = 0; i < NUM_KEYS; ++i) {
        freeKey(keys[i]);
    }
    hmFree(hm);

    puts("==== DONE HASHMAP ====");
}
