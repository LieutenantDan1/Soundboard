build:
	g++ -o sb main.cpp utils.cpp -std=c++20 -O3 -lsfml-audio
install: build
	mv sb /usr/bin