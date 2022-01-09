#include "hashmap.h"

#include <stdlib.h>
#include <assert.h>

int pairCmp(const void* a, const void* b) {
    return KeyCmp(((Pair*) a)->key, ((Pair*) b)->key);
}

SPECIALIZE_DYNARRAY_NAMED(KV, Pair, pairCmp)

// TODO: make implementation not suck
HashMap hmNew(size_t numBuckets) {
    HashMap hm;
    hm.buckets = (Array*) malloc(numBuckets*sizeof(Array));
    assert(hm.buckets != NULL); // TODO: handle this
    for (size_t i = 0; i < numBuckets; ++i) {
        hm.buckets[i] = arrNewKV();
    }
    hm.numBuckets = numBuckets;
    return hm;
}

HashMap hmCopy(HashMap hm) {
    HashMap copy;
    copy.buckets = (Array*) malloc(hm.numBuckets*sizeof(Array));
    assert(copy.buckets != NULL); // TODO: handle this
    for (size_t i = 0; i < hm.numBuckets; ++i) {
        copy.buckets[i] = arrCopyKV(hm.buckets[i]);
    }
    copy.numBuckets = hm.numBuckets;
    return copy;
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
