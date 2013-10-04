CC = g++
CFLAGS = -O3 -fopenmp
DEFINES =
LDFLAGS = -fopenmp -lm -lpthread -ldl

EXE = generate
RM = rm -rf
OBJ = generate.o index.o moves.o probe.o

$(EXE): $(OBJ)
	$(CC) $(DEFINES) $(LDFLAGS) $(OBJ) -o $(EXE)
%.o: %.cpp
	$(CC) $(CFLAGS) $(DEFINES) -c $<
clean:
	$(RM) $(OBJ)
