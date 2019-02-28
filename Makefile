CFLAGS=-Wall -Wextra

spiflasher: spiflasher.o
	$(CC) -o spiflasher spiflasher.o -lsoc  $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o spiflasher


