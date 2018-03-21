#pragma once

#include <stdbool.h>
#include <stdint.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

struct trie;

extern struct trie * trie_new(void);

extern bool trie_set(struct trie *, u32, u8*);

extern bool trie_get(struct trie *, u32, u8*);

extern bool trie_del(struct trie *, u32, u8*);
