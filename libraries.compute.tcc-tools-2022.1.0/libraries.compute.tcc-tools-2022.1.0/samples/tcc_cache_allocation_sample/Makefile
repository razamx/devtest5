all: tcc_cache_allocation_sample
tcc_cache_allocation_sample: main.o
	$(MAKE) -C ../common/ all
	$(CC) -o tcc_cache_allocation_sample main.o ../common/pointer_chasing.o -Wl,--no-as-needed  -ltcc -ltcc_cache -ltcc_static -ldl -littnotify -lpthread
main.o: src/main.c
	$(CC) -I "../common/include" -O2 -c src/main.c
clean:
	rm tcc_cache_allocation_sample *.o
