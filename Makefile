CC = g++
CFLAGS = -O3
DEFINES =
LDFLAGS = -fopenmp

EXE = generate
RM = rm -rf
OBJ = generate.o index.o moves.o probe.o

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $(OBJ) -o $(EXE) -lm -lpthread -ldl

%.o: %.cpp
	$(CC) $(CFLAGS) $(DEFINES) -c $<
clean:
	$(RM) $(OBJ)
