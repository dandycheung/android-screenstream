LDFLAGS += -lX11 -lpthread -ljpeg


main: main.o decode_jpeg.o

watch:
	nodemon -w main.c -d 0.2 -x "make main && ./main"

clean:
	rm -f main

.PHONY: clean

