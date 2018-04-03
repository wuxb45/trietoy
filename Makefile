.PHONY:all
all : replay.out memtest.out
replay.out : replay.c trie.c trie.h Makefile
	gcc -std=gnu11  -o replay.out replay.c trie.c
memtest.out : memtest.c trie.c trie.h Makefile
	gcc -std=gnu11 -o memtest.out memtest.c trie.c
