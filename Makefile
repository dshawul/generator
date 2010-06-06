CC = g++
CFLAGS = -O3
DEFINES =
LDFLAGS = 

RM = rm -rf
OBJ = generate.o index.o moves.o probe.o

egbbso.so: $(OBJ)
	$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $(OBJ) -shared -o egbbso.so -lm -lpthread

%.o: %.cpp
	$(CC) $(CFLAGS) $(DEFINES) -c -fPIC $<
clean:
	$(RM) $(OBJ)
