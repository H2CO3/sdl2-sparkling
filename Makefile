CC = clang
CXX = clang++
LD = $(CXX)

CFLAGS = -std=c99 -c -pedantic -O3 -flto -Wall -DUSE_DYNAMIC_LOADING
CXFLAGS = -std=c++11 -c -pedantic -O3 -flto -Wall -DUSE_DYNAMIC_LOADING
LDFLAGS = -dynamiclib \
          -L/usr/local/Cellar/sdl2_gfx/1.0.0/lib/ \
          $(shell sdl2-config --cflags --libs) \
          -O3 \
          -flto \
          -lspn \
          -lsdl2_gfx \
          -lsdl2_ttf \
          -lsdl2_image

TARGET = sdl2_spn.dylib
OBJECTS  = $(patsubst %.c, %.o, $(wildcard *.c))
OBJECTS += $(patsubst %.cpp, %.o, $(wildcard *.cpp))

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXFLAGS) -o $@ $<

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS)
