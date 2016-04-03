SRCDIR = src

INCLUDE = -I/usr/local/include $(shell sdl2-config --cflags)

CFLAGS = -std=c99 -c -pedantic -O3 -flto -Wall -DUSE_DYNAMIC_LOADING $(INCLUDE)
CXFLAGS = -std=c++11 -c -pedantic -O3 -flto -Wall -DUSE_DYNAMIC_LOADING $(INCLUDE)

LDFLAGS = -L/usr/local/lib/            \
		  $(shell sdl2-config --libs)  \
		  -O3                          \
		  -flto                        \
		  -lspn                        \
		  -lSDL2_gfx                   \
		  -lSDL2_ttf                   \
		  -lSDL2_image

ifeq ($(shell uname), Darwin)
	CC = clang
	CXX = clang++

	LDFLAGS += -dynamiclib

	EXT = dylib
else
	CC = gcc
	CXX = g++

	CFLAGS += -fPIC
	LDFLAGS += -shared

	EXT = so
endif

LD = $(CXX)

TARGET = sdl2.$(EXT)
OBJECTS  = $(patsubst $(SRCDIR)/%.c, %.o, $(wildcard $(SRCDIR)/*.c))
OBJECTS += $(patsubst $(SRCDIR)/%.cpp, %.o, $(wildcard $(SRCDIR)/*.cpp))

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXFLAGS) -o $@ $<

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS)
