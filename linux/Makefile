export ROOT_PATH := $(dir $(PWD))
LDFLAGS += -lX11 -lpthread -ljpeg
CFLAGS += -Wall -I$(ROOT_PATH)/shared $(APPEND_CFLAGS)

main: main.o decode.o

watch:
	nodemon -e .c,.h -d 0.2 -x "make main APPEND_CFLAGS=-DINPUT_DEBUG=1 && ./main || false"

clean:
	rm -f main *.o

keycode-gen: keycode-gen.c keycode-gen.h
	$(CC) -lX11 $(CFLAGS) -o $@ $<
	./keycode-gen

.PHONY: clean watch

