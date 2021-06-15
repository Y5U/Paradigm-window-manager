release: pwm.c config.h
	cc pwm.c -O3 -lX11 -lXrandr -lXinerama -o pwm

debug: pwm.c config.h
	cc pwm.c -D DEBUG -lX11 -lXrandr -lXinerama -Wall -Wextra -o pwm

install:
	cp pwm /usr/local/bin

uninstall:
	rm -f /usr/local/bin/pwm

clean:
	rm -f pwm

run:
	DISPLAY=:1 ./pwm
