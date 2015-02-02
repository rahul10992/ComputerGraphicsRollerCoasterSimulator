CC=g++

CFLAGS = -c -std=c++0x

GLFLAGS = -lglut -lGL -lGLU

all: RollerCoaster

RollerCoaster: RollerCoaster.o
	$(CC) RollerCoaster.o $(GLFLAGS) -o RollerCoaster

RollerCoaster.o: RollerCoaster.cpp
	$(CC) $(CFLAGS) RollerCoaster.cpp $(GLFLAGS)

clean:
	rm -rf *o RollerCoaster
