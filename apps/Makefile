CFLAGS += -O2 -Wshadow -Wpointer-arith -fPIC
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wnonnull -Wunused -Wuninitialized -fvisibility=hidden
CFLAGS += -g -Wall -Wextra -std=gnu99 -pipe -ggdb3 -I. -I..

LIBS += -lm
CC = gcc

EXTOBJS = ../lib/*.o
APPS = v4l2cat

GCNO = $(APPS:=.gcno)
GCDA = $(APPS:=.gcda)

all: $(APPS)

$(APPS): $(APPS).c
	$(CC) $(CFLAGS) -o $(APPS) $(APPS).c $(EXTOBJS) $(LIBS)

clean:
	$(RM) $(APPS) *.o


