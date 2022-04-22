all: project

project: project.cpp timers.cpp
	g++ project.cpp timers.cpp libggfonts.a -Wall -lGL -lGLU -lX11 -lpthread -oproject

clean:
	rm -f project


