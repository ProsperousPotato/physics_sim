all:
	gcc -o physics physics.c -lSDL2 -lm -O3

run: all
	setsid -f ./physics
