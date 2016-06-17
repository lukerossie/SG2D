
all:
	make -C ../../SG2D && g++ -static-libstdc++ -m32 ../src/main.cpp ../../SG2D/libSDL2W.a -lSDL2 -lSDL2_image -lSDL2_net -lSDL2_ttf -lSDL2_mixer -std=c++1y -g -O0 -o ../release_linux/main && ../release_linux/main
#remove debugging symbols (-g) for release, enable optimization (-0x)

