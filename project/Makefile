# Project: BB8

CPP  = g++
CC   = gcc
BIN  = BB8_Cinelli

OBJ  = main.o bb8.o mesh.o
LINKOBJ  = main.o bb8.o mesh.o
LIBS = -L/usr/X11R6 -lGL -lGLU -lSDL2_image -lSDL2 -lm -lSDL2_ttf -lglut -lGL -lGLU
INCS = -I. -I/usr/X11R6/include
CXXINCS=#
CXXFLAGS = $(CXXINCS)
CFLAGS = $(INCS)
RM = rm -f

all: $(BIN)

clean:
	${RM} $(OBJ) $(BIN)

main.o: main.cpp
	$(CPP) -c main.cpp -o main.o $(CXXFLAGS)

bb8.o: bb8.cpp
	$(CPP) -c bb8.cpp -o bb8.o $(CXXFLAGS)

mesh.o: mesh.cpp
	$(CPP) -c mesh.cpp -o mesh.o $(CXXFLAGS)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)
