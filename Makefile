CC=gcc
CFLAGS=-Wall

# Main target
zuki: main.o zukiC.o cJSON.o
	$(CC) $(CFLAGS) $^ -o $@

# Dependencies and compilation rules
main.o: main.c cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

zukiC.o: zuki.c cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

cJSON.o: cJSON.c cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o zuki