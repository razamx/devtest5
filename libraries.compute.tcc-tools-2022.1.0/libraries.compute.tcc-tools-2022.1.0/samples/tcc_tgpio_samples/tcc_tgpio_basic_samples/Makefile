all: tcc_tgpio_basic_input tcc_tgpio_basic_oneshot_output tcc_tgpio_basic_periodic_output tcc_tgpio_info

tcc_tgpio_basic_oneshot_output: tgpio_basic_oneshot_output.o 
	$(CC) -o tcc_tgpio_basic_oneshot_output tgpio_basic_oneshot_output.o 
tgpio_basic_oneshot_output.o: src/tgpio_basic_oneshot_output.c
	$(CC) -O2 -I "./../../common/include" -c src/tgpio_basic_oneshot_output.c

tcc_tgpio_basic_input: tgpio_basic_input.o 
	$(CC) -o tcc_tgpio_basic_input tgpio_basic_input.o 
tgpio_basic_input.o: src/tgpio_basic_input.c
	$(CC) -O2 -I "./../../common/include" -c src/tgpio_basic_input.c

tcc_tgpio_basic_periodic_output: tgpio_basic_periodic_output.o 
	$(CC) -o tcc_tgpio_basic_periodic_output tgpio_basic_periodic_output.o 
tgpio_basic_periodic_output.o: src/tgpio_basic_periodic_output.c
	$(CC) -O2  -I "./../../common/include" -c src/tgpio_basic_periodic_output.c

tcc_tgpio_info: tgpio_info.o
	$(CC) -o tcc_tgpio_info tgpio_info.o
tgpio_info.o: src/tgpio_info.c
	$(CC) -O2  -I "./../../common/include" -c src/tgpio_info.c
clean:
	rm tcc_tgpio_basic_input tcc_tgpio_basic_oneshot_output tcc_tgpio_basic_periodic_output tcc_tgpio_info  *.o
