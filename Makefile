all:
	clang -c -std=c99 -pedantic -Wall -o sdl2_sparkling.o -DUSE_DYNAMIC_LOADING sdl2_sparkling.c -O3 -flto
	clang++ -c -std=c++11 -pedantic -Wall -o ttf_support.o ttf_support.cpp -O3 -flto
	clang++ -dynamiclib -L/usr/local/Cellar/sdl2_gfx/1.0.0/lib/ -lsdl2_gfx -lsdl2_ttf -lspn $(shell sdl2-config --cflags --libs) -O3 -flto -o sdl2_spn.dylib sdl2_sparkling.o ttf_support.o

clean:
	rm -f sdl2_spn.dylib
	rm *.o
