#CC = g++
#CFLAGS = -O3 -Wno-unused-result
#LDFLAGS = -fopenmp -lm -lpthread -ldl

CC = x86_64-w64-mingw32-g++
CFLAGS = -O3 -Wno-unused-result -static
LDFLAGS = -fopenmp -lm -lpthread

DEFINES =

EXE = generate
RM = rm -rf
OBJ = generate.o index.o moves.o probe.o

$(EXE): $(OBJ)
	$(CC) $(OBJ) $(DEFINES) $(LDFLAGS) -o $(EXE)
%.o: %.cpp
	$(CC) $(CFLAGS) $(DEFINES) -c $<
clean:
	$(RM) $(OBJ) $(EXE)
