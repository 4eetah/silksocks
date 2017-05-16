SRCS := main.c urlqueue.c href.c
HDRS := urlqueue.h common.h
OBJS := $(SRCS:.c=.o)
OUT := socksload
LIBS := -lpcre2-8 -lcurl
CFLAGS := -O0 -g -L ../../pcre2-10.23/.libs -I ../../pcre2-10.23/src -pthread
#CFLAGS := -p -g -O0 -DDEBUG -DUSEDB

all: $(OUT)

tags: *.c *.h
	ctags -R

$(OUT): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(OUT) *.o 
