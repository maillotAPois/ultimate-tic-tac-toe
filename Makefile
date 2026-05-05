CXX      := x86_64-w64-mingw32-g++
CC       := x86_64-w64-mingw32-gcc
CXXFLAGS := -std=c++17 -O2 -fno-lto -Wall -Wextra
LDFLAGS  := -static -static-libgcc -static-libstdc++
INCLUDES := -I. -Isrc
LIBS     := -L. -lUTTTLib

SRCS     := main.cpp src/Board.cpp src/GameState.cpp src/MinimaxPlayer.cpp src/MCTSPlayer.cpp src/UltimateBoard.cpp
STUBS_C  := build/allegro_stubs.c
STUBS_O  := build/allegro_stubs.o
TARGET   := uttt.exe

WINE     := WINEDEBUG=-all wine

.PHONY: all run clean stubs

all: $(TARGET)

$(STUBS_C): libUTTTLib.a
	@mkdir -p build
	@$(CC) -dumpversion >/dev/null
	@x86_64-w64-mingw32-nm $< | awk '$$1=="U"{print $$2}' | sort -u | grep '^al_' \
		| awk '{print "int " $$0 "() { return 1; }"}' > $@
	@echo "stubs regenerated: $@"

$(STUBS_O): $(STUBS_C)
	$(CC) -c $< -o $@

$(TARGET): $(SRCS) $(STUBS_O) libUTTTLib.a
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(STUBS_O) $(LIBS) $(LDFLAGS) -o $@

run: $(TARGET)
	$(WINE) ./$(TARGET)

clean:
	rm -rf build $(TARGET)
