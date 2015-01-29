CC=gcc
FLAGS=-wg
CFLAGS=-lcrypto -lm


SOURCE=bencode.c  bt_client.c bt_lib.c bt_setup.c
HDR=bencode.h bt_lib.h
OBJECT=$(SOURCE:.c=.o)
BINARY=bt_client

$(BINARY) : $(SOURCE) $(HDR)
			$(CC) $(FLAGS) $(CFLAGS) -o $(BINARY) $(SOURCE)


%.o:%.c
		$(CC) -c $(CFLAGS) -o $@ $<
