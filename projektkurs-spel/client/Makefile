INCLUDE = C:\msys64\mingw64\include
LDFLAGS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2_net -mwindows -lm

spelareClient: main.o zombie.o spelare.o music.o bullet.o text.o
	gcc -o spelareClient main.o zombie.o spelare.o music.o bullet.o text.o $(LDFLAGS)

main.o: ./src/main.c
	gcc -c ./src/main.c

zombie.o: ../lib/src/zombie.c ../lib/include/zombie.h
	gcc -c ../lib/src/zombie.c

spelare.o: ../lib/src/spelare.c ../lib/include/spelare.h
	gcc -c ../lib/src/spelare.c

music.o: ../lib/src/music.c ../lib/include/music.h
	gcc -c ../lib/src/music.c

bullet.o: ../lib/src/bullet.c ../lib/include/bullet.h
	gcc -c ../lib/src/bullet.c

text.o: ../lib/src/text.c ../lib/include/text.h
	gcc -c ../lib/src/text.c

clean:
	rm *.o
	rm *.exe