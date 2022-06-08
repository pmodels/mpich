CC = mpicc
LD = mpecc
OBJ = ring.o
SRC = ring.c
EXEC = ring
CFLAGS = -g
LDFLAGS = -lm -mpilog

all: $(EXEC)

$(EXEC): ${OBJ}
	$(LD) $(LDFLAGS) $< -o $(EXEC)

ring.o: ${SRC}
	$(CC) $(CFLAGS) -c ring.c

clean:
	$(RM) ${OBJ} $(EXEC)
	$(RM) *.clog2 *.slog2
