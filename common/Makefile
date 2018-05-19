CFLAGS += -O2 -Wshadow -Wpointer-arith -fPIC
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
CFLAGS += -Wnonnull -Wunused -Wuninitialized -fvisibility=hidden

CFLAGS += -Wall -Wextra -std=gnu99 -pipe -ggdb3 -I. -I..
LIBS += -lz -lJudy
CC = gcc

OBJS = v4l2cap.o
SRCS = $(OBJS:.o=.c)
HDRS = $(OBJS:.o=.h)
DEPS = $(OBJS:.o=.d)
GCNO = $(OBJS:.o=.gcno)
GCDA = $(OBJS:.o=.gcda)

target: $(OBJS) libv4l2cap.a

%.o: %.c
	$(CC) $(CFLAGS) -MQ $@ -MMD -MP -c -o $@ $<

libv4l2cap.a: $(OBJS)
	$(AR) r $@ $?

.PHONY : clean
clean:
	rm -f *.d *.o *.gcno *.gcda libv4l2cap.a

