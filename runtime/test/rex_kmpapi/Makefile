parallel: parallel.c
	g++ -c $< -I../../../build/runtime/src -std=c++11
	g++ $@.o -L../../../build/runtime/src -lomp -o $@

rex_for: rex_for.c
	g++ -c $< -I../../../build/runtime/src -std=c++11
	g++ $@.o -L../../../build/runtime/src -lomp -o $@

rex_fib: rex_fib.c
	g++ -c $< -I../../../build/runtime/src -std=c++11
	g++ $@.o -L../../../build/runtime/src -lomp -o $@

clean:
	rm -rf parallel rex_for rex_fib *.o
