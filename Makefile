kiloe: kiloe.c
	$(CC) kiloe.c -o kiloe -Wall -Wextra -pedantic -std=c99

clean:
	rm -f kiloe

.PHONY: clean