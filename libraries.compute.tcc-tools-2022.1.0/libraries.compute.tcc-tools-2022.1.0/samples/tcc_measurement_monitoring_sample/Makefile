all: tcc_measurement_monitoring_sample
tcc_measurement_monitoring_sample: main.o
	$(CC) -I "./../common/include" -o tcc_measurement_monitoring_sample main.o -ltcc
main.o: src/main.c
	$(CC) -I "./../common/include" -O2 -c src/main.c
clean:
	rm tcc_measurement_monitoring_sample *.o
