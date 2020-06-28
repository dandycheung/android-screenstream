CPPFLAGS += -lX11 -lpthread


main: main.c

watch:
	nodemon -w main.c -d 0.2 -x "make main && ./main"

clean:
	rm -f main

.PHONY: clean

