all: tcc_frequency_discipline_sample

tcc_frequency_discipline_sample: main.o
	$(MAKE) -C ../../common/ all
	$(CC) -o tcc_frequency_discipline_sample main.o ../../common/time_convert.o ../../common/tgpio.o
main.o: src/main.c
	$(CC) -I "./../../common/include" -O2 -c src/main.c
clean:
	rm tcc_frequency_discipline_sample *.o
