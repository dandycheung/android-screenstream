#pragma once

extern int fd_input;
extern struct input_event *event_key, *event_sync;
extern uint8_t event_size;

void input_setup(uint16_t port);
int input_press(int key);