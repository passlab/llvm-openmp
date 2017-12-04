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

omp: test_omp_parallel test_omp_for test_omp_fib

test_omp_parallel: test_omp_parallel.c
	clang -fopenmp -emit-llvm -S $< 
	clang -fopenmp $< -o $@ 

test_omp_for: test_omp_for.c
	clang -fopenmp -emit-llvm -S $< 
	clang -fopenmp $< -o $@ 

test_omp_fib: test_omp_fib.c
	clang -fopenmp -emit-llvm -S $< 
	clang -fopenmp $< -o $@ 

clean:
	rm -rf test_rex_parallel test_rex_for test_rex_fib *.o test_omp_parallel test_omp_for test_omp_fib  *.ll
