# Makefile for Dun-geon
# Created by Matyas Cerny 2016/12/20

PROG = dun-geon
win: PROG = dun-geon.exe
SRCDIR = ./src
OBJDIR = ./obj
CC = clang++
win: CC = i686-w64-mingw32-g++
CFLAGS = -std=c++11 -Wall -pedantic
win: CFLAGS = -std=c++11 -Wall -pedantic -static-libgcc -static-libstdc++ -Wno-sign-compare
OBJS = main.o character.o entity.o room.o screen.o player.o utils.o nonport.o world.o parser.o item.o
NONPORT = nonport.cpp
win: NONPORT = nonport_win.cpp

# Compiling for Linux
linux: ./obj $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(addprefix $(OBJDIR)/,$(OBJS))

# Compiling for Windows
win: ./obj $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(addprefix $(OBJDIR)/,$(OBJS))

./obj:
	mkdir $(OBJDIR)

main.o: $(SRCDIR)/main.cpp
	$(CC) $(CFLAGS) -c $(SRCDIR)/main.cpp -o $(OBJDIR)/main.o

character.o: $(SRCDIR)/character.cpp $(SRCDIR)/character.h $(SRCDIR)/entity.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/character.cpp -o $(OBJDIR)/character.o

entity.o: $(SRCDIR)/entity.cpp $(SRCDIR)/entity.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/entity.cpp -o $(OBJDIR)/entity.o

room.o: $(SRCDIR)/room.cpp $(SRCDIR)/room.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/room.cpp -o $(OBJDIR)/room.o

screen.o: $(SRCDIR)/screen.cpp $(SRCDIR)/screen.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/screen.cpp -o $(OBJDIR)/screen.o

player.o: $(SRCDIR)/player.cpp $(SRCDIR)/player.h $(SRCDIR)/character.h $(SRCDIR)/entity.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/player.cpp -o $(OBJDIR)/player.o

world.o: $(SRCDIR)/world.cpp $(SRCDIR)/world.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/world.cpp -o $(OBJDIR)/world.o

nonport.o: $(SRCDIR)/$(NONPORT)
	$(CC) $(CFLAGS) -c $(SRCDIR)/$(NONPORT) -o $(OBJDIR)/nonport.o

utils.o: $(SRCDIR)/utils.cpp $(SRCDIR)/utils.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/utils.cpp -o $(OBJDIR)/utils.o

parser.o: $(SRCDIR)/parser.cpp $(SRCDIR)/parser.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/parser.cpp -o $(OBJDIR)/parser.o

item.o: $(SRCDIR)/item.cpp $(SRCDIR)/item.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/item.cpp -o $(OBJDIR)/item.o

.PHONY: clean
clean:
	rm -rf $(OBJDIR) ./src/*~ *~ dun-geon dun-geon.exe ./data/controls.conf
