#include "hashmap.h"

#include <stdlib.h>
#include <assert.h>

// best results with prime numBuckets
HashMap hmNew(size_t numBuckets) {
    HashMap hm;
    hm.buckets = (Array*) malloc(numBuckets*sizeof(Array));
    for (size_t i = 0; i < numBuckets; ++i) {
        hm.buckets[i] = arrNewKV();
    }
    hm.numBuckets = numBuckets;
    return hm;
}

void hmFree(HashMap hm) {
    for (size_t i = 0; i < hm.numBuckets; ++i) {
        arrFree(hm.buckets[i]);
    }
    free(hm.buckets);
}

Hint hmFind(HashMap* hm, Pair x) {
    Hint h;
    h.hv = KeyHash(x.key) % hm->numBuckets;
    h.found = arrIndexKV(&hm->buckets[h.hv], x, &h.idx);
    return h;
}

Pair* hmHintGet(HashMap* hm, Hint h) {
    if (!h.found) {
        return NULL;
    }
    return arrGetKV(&hm->buckets[h.hv], h.idx);
}

Pair* hmGet(HashMap* hm, Key k) {
    return hmHintGet(hm, hmFind(hm, (Pair) { .key = k }));
}

bool hmHintPut(HashMap* hm, Hint h, Pair x) {
    if (h.found) {
        return false; // already exists
    }
    if (!arrPushKV(&hm->buckets[h.hv], x)) {
        assert(0 && "uh oh bad array resize");
    }
    return true;
}

bool hmPut(HashMap* hm, Pair x) {
    return hmHintPut(hm, hmFind(hm, x), x);
}

bool hmHintRemove(HashMap* hm, Hint h) {
    if (!h.found) {
        return false;
    }
    return arrEraseKV(&hm->buckets[h.hv], h.idx, 1);
}

bool hmRemove(HashMap* hm, Key k) {
    return hmHintRemove(hm, hmFind(hm, (Pair) { .key = k }));
}
