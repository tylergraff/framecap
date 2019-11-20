.PHONY: target clean

CSRCAPPS:=$(wildcard apps/*.c)
APPS:=$(CSRCAPPS:.c=)

CSRCCOMMON:=$(wildcard common/*.c)
CHDRCOMMON:=$(wildcard common/*.h)

OBJCOMMON:=$(CSRCCOMMON:.c=.o)
LIBCOMMON:=common/libutil.a

CHDRS:=$(CHDRCOMMON)

CC = gcc
LIBS += -lJudy -lpthread -lm -lz

target: $(APPS) $(LIBCOMMON)

apps/%: apps/%.c $(OBJCOMMON)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(LIBCOMMON): $(OBJCOMMON)
	ar r $@ $^

%.o: %.c $(CHDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(LIBCOMMON) $(OBJCOMMON)
	rm -f $(APPS)
