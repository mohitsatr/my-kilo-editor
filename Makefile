testing: testing.c
	$(CC) testing.c -o testing -Wall -Wextra -pedantic -std=c89
kilo: kilo.c
	$(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c89
	./kilo


