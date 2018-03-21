replay.out : replay.c trie.c trie.h Makefile
	gcc -std=gnu11  -o replay.out replay.c trie.c
