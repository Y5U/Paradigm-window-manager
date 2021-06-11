#pragma once

typedef struct {
	unsigned int state; /* modifier keys */
	KeySym keysym;
	void (*func)(int args[10]);
	int args[10];
} Key;
