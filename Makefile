release: pwm.c pwm.h config.h
	cc pwm.c -O3 -lX11 -lXrandr -lXinerama -lpthread -o pwm

debug: pwm.c pwm.h config.h
	cc pwm.c -D DEBUG -lX11 -lXrandr -lXinerama -lpthread -Wall -Wextra -o pwm

install:
	cp pwm /usr/bin

clean:
	rm -f pwm

run:
	DISPLAY=:1 ./pwm
