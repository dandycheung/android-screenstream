LDFLAGS += -lX11 -lpthread -ljpeg
CFLAGS += -Wall

main: main.o decode.o

watch:
	nodemon -e .c,.h -d 0.2 -x "make main && ./main || false"

clean:
	rm -f main *.o

.PHONY: clean watch

