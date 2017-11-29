default: test_rex_parallel test_rex_for test_rex_fib

test_rex_parallel: test_rex_parallel.c
	g++ -c $< -I../../../build/runtime/src -std=c++11
	g++ $@.o -L../../../build/runtime/src -lomp -o $@

test_rex_for: test_rex_for.c
	g++ -c $< -I../../../build/runtime/src -std=c++11
	g++ $@.o -L../../../build/runtime/src -lomp -o $@

test_rex_fib: test_rex_fib.c
	g++ -c $< -I../../../build/runtime/src -std=c++11
	g++ $@.o -L../../../build/runtime/src -lomp -o $@

clean:
	rm -rf test_rex_parallel test_rex_for test_rex_fib *.o
