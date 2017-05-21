CFLAGS := -I../ -DDEBUG=1
OBJS := ../cache.o ../gf_time.o ../error.o ../connect_nonb.o ../host_serv.o ../cache.o ../hashtbl.c
OUT := test_cache test_connect_nonb test_htbl

all: $(OUT) 

test_cache: test_cache.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_connect_nonb: test_connect_nonb.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_htbl: test_htbl.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(OUT) *.o
