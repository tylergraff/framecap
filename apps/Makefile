CFLAGS += -O2 -Wshadow -Wpointer-arith -fPIC
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wnonnull -Wunused -Wuninitialized -fvisibility=hidden
CFLAGS += -g -Wall -Wextra -std=gnu99 -pipe -ggdb3 -I. -I..

LIBS += -lm
CC = gcc

EXTOBJS = ../common/*.o
APPS = vcat yuyv-util

GCNO = $(APPS:=.gcno)
GCDA = $(APPS:=.gcda)

all: $(APPS)

v4l2cat: vcat.c
	$(CC) $(CFLAGS) -o vcat vcat.c $(EXTOBJS) $(LIBS)

yuyv-util: yuyv-util.c
	$(CC) $(CFLAGS) -o yuyv-util yuyv-util.c $(EXTOBJS) $(LIBS)

clean:
	$(RM) $(APPS) *.o


