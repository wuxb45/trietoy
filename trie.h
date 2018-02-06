#pragma once

#include "lib1.h"

struct trie;

// Create a trie instance and return a pointer to the new trie.
  extern struct trie *
trie_new(void);

// Destroy a trie instance. Note to free any memory associated with _t_.
  extern void
trie_destroy(struct trie * const t);

// "SET" the given _kv_ in _t_.
// _t_ will make a private copy of _kv_; caller can free _kv_ after the call is returned.
// Return true on success.
// Return false if any error happens and the operation did not complete (e.g., no memory).
  extern bool
trie_set(struct trie * const t, const struct kv * const kv);

// "DEL" a corresponding key in _t_ if the given _key_ is found.
// The value part of _key_ is ignored in this function.
// Return true if the item is found and deleted.
// Return false if nothing is found and nothing is deleted.
  extern bool
trie_del(struct trie * const t, const struct kv * const key);

// "GET" a kv from _t_ if the given _key_ exists.
// The value part of _key_ is ignored in this function.
// If found, the result kv is copied into _out_, the pointer to _out_ is also returned.
// If not found, a NULL-pointer is return and _out_ is kept unchanged.
  extern struct kv *
trie_get(struct trie * const t, const struct kv * const key, struct kv * const out);
