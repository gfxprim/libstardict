LIB=stardict
LIB_SRCS=libstardict.c
LIB_LDLIBS=-lz
LIB_HEADERS=$(wildcard *.h)

BIN_SRCS=$(BIN).c
BIN=sd-cmd
