all:
	g++ -c SG2D.cpp src/util.cpp src/core.cpp src/graphics.cpp src/input.cpp src/net.cpp src/sound.cpp -fpic -std=c++1y -g && ar -cr libSDL2W.a *.o && rm *.o
#remove debugging symbols (-g) for release, enable optimization (-0x)
