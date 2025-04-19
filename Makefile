CC=gcc
CFLAGS=-Wall
LIBS=-lcurl

# Main target
zuki: main.o zukiC.o cJSON.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# Dependencies and compilation rules
main.o: main.c cJSON/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

zukiC.o: zukiC/zuki.c cJSON/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@ 

cJSON.o: cJSON/cJSON.c cJSON/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o zuki
