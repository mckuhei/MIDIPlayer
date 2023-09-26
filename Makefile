CPP      = g++.exe
CC       = gcc.exe
WINDRES  = windres.exe
LINKOBJ  = cui.o player.o winmm_interface.o util.o
OBJ      = $(LINKOBJ)
LIBS     = -lwinmm
INCS     = 
CXXINCS  = 
BIN      = main.exe
CXXFLAGS = $(CXXINCS) -m64 -std=c++11 -O2
CFLAGS   = $(INCS) -m64 -std=c11 -O2
RM		= del /q 

.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean-custom:
	

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(LINKOBJ)
	$(CC) $(LINKOBJ) -o $(BIN) $(LIBS)

%.o: %.cc
	$(cc) -c $(notdir $(basename $@)).cc $(CXXFLAGS) -o $@