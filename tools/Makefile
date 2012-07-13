CC=gcc

CFLAGS=-Wall -pedantic -pipe -O2

.PHONY: all

all: glfpatch

glfpatch: glfpatch.c
	${CC} ${CFLAGS} -o $@ $<
