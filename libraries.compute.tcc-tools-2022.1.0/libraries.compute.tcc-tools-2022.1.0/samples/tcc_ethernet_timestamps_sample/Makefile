all: tcc_ethernet_timestamps_sample
tcc_ethernet_timestamps_sample: main.o
	$(MAKE) -C ../common/ all
	$(CC) -o tcc_ethernet_timestamps_sample main.o ../common/time_convert.o ../common/tgpio.o ../common/ui.o -lm
main.o: src/main.c
	$(CC) -I "./../common/include" -O2 -c src/main.c
clean:
	rm tcc_ethernet_timestamps_sample *.o
