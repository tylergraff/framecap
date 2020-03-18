.DELETE_ON_ERROR:

#GITVERSION := $(shell ./gitversion.rb)

.PHONY: target clean test

PARALLEL:=1

CSRCAPPS:=$(wildcard apps/*.c)
APPS:=$(CSRCAPPS:.c=)

CSRC:=$(wildcard *.c)
CHDR:=$(wildcard *.h)
OBJS:=$(CSRC:.c=.o)
LIBFRAMECAP:=libframecap.a

#RUBYEXTS:=ext/notos.so

CHDRS:=$(CHDR)

CFLAGS += -Wshadow -Wpointer-arith -fPIC
CFLAGS += -Wcast-qual
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wnonnull -Wunused -Wuninitialized -fvisibility=hidden
CFLAGS += -Werror -Wall -Wextra -std=gnu99 -pipe -ggdb3 -I.
CFLAGS += -I/usr/local/include -L/usr/local/lib
CFLAGS += -Wno-format-truncation
CFLAGS += -D_GNU_SOURCE
LIBS += -lpthread -lz -lm
CC=gcc

target: $(APPS) $(LIBFRAMECAP)

GCNO:=$(OBJS:.o=.gcno)
GCDA:=$(OBJS:.o=.gcda)

apps/%: apps/%.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(LIBFRAMECAP): $(OBJS)
	ar r $@ $^

libframecap.so: $(OBJS)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJS) $(LIBS)

%.o: %.c $(CHDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(LIBFRAMECAP) $(OBJS)
	rm -f $(GCNO) $(GCDA) *.gcno *.gcda
	rm -f $(APPS)

test: target
	true

.PHONY : symbols
