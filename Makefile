CXX      := x86_64-w64-mingw32-g++
CC       := x86_64-w64-mingw32-gcc
CXXFLAGS := -std=c++17 -O2 -fno-lto -Wall -Wextra
LDFLAGS  := -static -static-libgcc -static-libstdc++
INCLUDES := -I. -Isrc
LIBS     := -L. -lUTTTLib

SRCS     := main.cpp src/Board.cpp src/GameState.cpp src/MinimaxPlayer.cpp src/UltimateBoard.cpp
STUBS_C  := build/allegro_stubs.c
STUBS_O  := build/allegro_stubs.o
TARGET   := uttt.exe

WINE     := WINEDEBUG=-all wine

.PHONY: all run clean stubs selfplay

all: $(TARGET)

# Harnais de self-play (moteur A vs moteur B), compile en natif: pas de
# framework, pas de wine, deterministe -> mesure A/B a bruit nul.
SELFPLAY_SRCS := selfplay.cpp src/Board.cpp src/GameState.cpp \
                 src/UltimateBoard.cpp src/MinimaxPlayer.cpp
selfplay: $(SELFPLAY_SRCS)
	clang++ -std=c++17 -O2 $(INCLUDES) $(SELFPLAY_SRCS) -o selfplay

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
	rm -rf build $(TARGET) selfplay
