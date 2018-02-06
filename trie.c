#include "trie.h"

struct trie {
  u64 nr_alloc;
  u64 nr_real;
  struct kv ** array;
};


  struct trie *
trie_new(void)
{
  struct trie * const t = malloc(sizeof(*t));
  if (t == NULL) return NULL;
  t->array = malloc(sizeof(struct kv *) * 256);
  if (t->array == NULL) {
    free(t);
    return NULL;
  }
  t->nr_alloc = 256;
  t->nr_real = 0;
  return t;
}

  void
trie_destroy(struct trie * const t)
{
  if (t == NULL || t->array == NULL) return;
  for (u64 i = 0; i < t->nr_real; i++) {
    free(t->array[i]);
  }
  free(t->array);
  free(t);
}

  bool
trie_set(struct trie * const t, const struct kv * const kv)
{
  if (t == NULL || t->array == NULL) return false;
  if (kv == NULL) return false;
  // try replace
  for (u64 i = 0; i < t->nr_real; i++) {
    if (kv_keymatch(kv, t->array[i])) {
      free(t->array[i]);
      t->array[i] = kv_dup(kv);
      return true;
    }
  }
  // may resize array
  if (t->nr_real == t->nr_alloc) {
    struct kv ** const newarray = realloc(t->array, sizeof(t->array[0]) * t->nr_alloc * 2);
    if (newarray == NULL) return false;
    t->array = newarray;
    t->nr_alloc *= 2;
  }
  // append
  t->array[t->nr_real] = kv_dup(kv);
  if (t->array[t->nr_real] == NULL) {
    return false;
  }
  t->nr_real++;
  return true;
}

  bool
trie_del(struct trie * const t, const struct kv * const key)
{
  if (t == NULL || t->array == NULL) return false;
  if (key == NULL) return false;
  // find it and free it, fill the gap with the tail.
  for (u64 i = 0; i < t->nr_real; i++) {
    if (kv_keymatch(key, t->array[i])) {
      free(t->array[i]);
      t->array[i] = NULL;
      t->array[i] = t->array[t->nr_real - 1];
      t->nr_real--;
      return true;
    }
  }
  return false;
}

  struct kv *
trie_get(struct trie * const t, const struct kv * const key, struct kv * const out)
{
  if (t == NULL || t->array == NULL) return NULL;
  if (key == NULL) return NULL;
  if (out == NULL) return NULL;
  for (u64 i = 0; i < t->nr_real; i++) {
    if (kv_keymatch(key, t->array[i])) {
      kv_dup2(t->array[i], out);
      return out;
    }
  }
  return NULL;
}
