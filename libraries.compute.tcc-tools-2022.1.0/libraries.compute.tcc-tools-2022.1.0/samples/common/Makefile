all: tgpio.o swgpio.o time_convert.o ui.o pointer_chasing.o

tgpio.o: src/tgpio.c include/tgpio.h
	$(CC) -I "./include" -O2 -c src/tgpio.c
swgpio.o: src/swgpio.c include/swgpio.h
	$(CC) -I "./include" -O2 -c src/swgpio.c
time_convert.o: src/time_convert.c include/time_convert.h
	$(CC) -I "./include" -O2 -c src/time_convert.c
ui.o: src/ui.c include/ui.h
	$(CC) -I "./include" -O2 -c src/ui.c
pointer_chasing.o: src/pointer_chasing.c include/pointer_chasing.h
	$(CC) -fPIC -I "./include" -O2 -c src/pointer_chasing.c
clean:
	rm *.o
