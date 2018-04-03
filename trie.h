#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

struct trie;

extern struct trie * trie_new(void);

// return true if a new key is successfully inserted
// return false if the keys is already there OR the insertion failed due to any problem (can't allocate memory, etc..)
extern bool trie_set(struct trie *, u32, u8*);

// return if the key is found in the trie
extern bool trie_get(struct trie *, u32, u8*);

// return true if a key is deleted
// return false if the key is not found
extern bool trie_del(struct trie *, u32, u8*);
