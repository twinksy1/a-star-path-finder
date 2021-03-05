flags=-lX11 -lm -lXext

all: main

main: main.cpp
	g++ $< -o $@ ${flags}

clean:
	rm -f main
