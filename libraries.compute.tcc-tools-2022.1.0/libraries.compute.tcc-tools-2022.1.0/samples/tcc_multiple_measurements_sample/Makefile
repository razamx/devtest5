all: tcc_multiple_measurements_sample
tcc_multiple_measurements_sample: main.o
	$(MAKE) -C ../common/ all
	$(CC) -o tcc_multiple_measurements_sample main.o -Wl,--no-as-needed  -ltcc -littnotify -ltcc_static -ldl -lm
main.o: src/main.c
	$(CC) -I "../common/include"  -O2 -c src/main.c
clean:
	rm tcc_multiple_measurements_sample *.o
