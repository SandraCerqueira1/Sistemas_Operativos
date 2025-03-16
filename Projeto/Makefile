CC = gcc
CFLAGS = -Iinclude
DEPS = include/client.h include/orchestrator.h

# Regra para construir todos os executáveis
all: obj/client obj/orchestrator

# Regras para compilar client
obj/client.o: src/client.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

obj/client: obj/client.o
	$(CC) -o $@ $^

# Regras para compilar orchestrator
obj/orchestrator.o: src/orchestrator.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

obj/orchestrator: obj/orchestrator.o
	$(CC) -o $@ $^

.PHONY: clean all

clean:
	rm -f obj/*.o obj/client obj/orchestrator
