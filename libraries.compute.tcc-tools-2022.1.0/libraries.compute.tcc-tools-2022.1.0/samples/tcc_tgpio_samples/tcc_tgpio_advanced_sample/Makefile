all: tcc_tgpio_advanced_sample

tcc_tgpio_advanced_sample: main.o
	$(MAKE) -C ../../common/ all
	$(CC) -o tcc_tgpio_advanced_sample main.o ../../common/swgpio.o ../../common/tgpio.o ../../common/time_convert.o
main.o: src/main.c
	$(CC) -I "./../../common/include" -O2 -c src/main.c
clean:
	rm tcc_tgpio_advanced_sample *.o
