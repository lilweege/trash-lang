#ifndef HASHMAP_H
#define HASHMAP_H

#include "dynarray.h"
#include "analyzer.h"

// Pair contains a Key type called key
typedef StringView Key;
#define KeyCmp svCmp
#define KeyHash svHash
typedef Symbol Pair;
#define key id


typedef struct {
    uint32_t hv;
    bool found;
    size_t idx;
} Hint;

typedef struct {
    Array* buckets;
    size_t numBuckets;
} HashMap;

HashMap hmNew(size_t numBuckets);
void hmFree(HashMap hm);
Hint hmFind(HashMap* hm, Pair x);
Pair* hmHintGet(HashMap* hm, Hint h);
bool hmHintPut(HashMap* hm, Hint h, Pair x);
bool hmHintRemove(HashMap* hm, Hint h);
Pair* hmGet(HashMap* hm, Key k);
bool hmPut(HashMap* hm, Pair x);
bool hmRemove(HashMap* hm, Key x);

#endif // HASHMAP_H
