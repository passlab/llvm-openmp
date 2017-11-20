# REX KMP API to LLVM OpenMP Runtime
This branch, rex_kmpapi, contains runtime API for the LLVM OpenMP runtime. 
Those APIs are in the form of rex_* API for the kmp functionality so users can directly 
  write their OpenMP style code using runtime API, i.e. without the need of compiler.

The [runtime/CMakeLists.txt](runtime/CMakeLists.txt) and [runtime/src/CMakeLists.txt](runtime/src/CMakeLists.txt) 
define and use LIBOMP_REX_KMPAPI_SUPPORT cmake variables to configure which files will be built into final library 
and what macros will be enabled (check [runtime/src/kmp_config.h.cmake](runtime/src/kmp_config.h.cmake) file). 

## Build the Library and Installation
  1. clone the repo and checkout the branch you need (only need to do once, mostly)
  
           git clone https://github.com/passlab/llvm-openmp 
           cd llvm-openmp
           git remote update
           git checkout -t origin/rex_kmpapi  # check out rex_kmpapi branch that tracks the same remote branch from origin
          
  1. `cmake` to create the makefile with the LIBOMP_* setting you want and `make` to build the library and the header files
  
           mkdir build
           cd build
           cmake -G "Unix Makefiles" -DLIBOMP_REX_KMPAPI_SUPPORT=on ..
           make
           
     The `make` command will build the runtime source codes into a library, the `libomp.so` file and create the header files (`omp.h` and `rex_kmp.h`) for the library. Those files are located in `runtime/src` folder under the `build` folder you are working on. 
           
     For using other compiler (on fornax), `CC` and `CXX` should be set for `cmake`. For example, on fornax: 
      
           CC=/opt/gcc-5.3.0-install/bin/gcc CXX=/opt/gcc-5.3.0-install/bin/g++ cmake -DLIBOMP_REX_KMPAPI_SUPPORT=on ..
           
     Each time you change the source code, either `rex_kmp.h` or `rex_kmp.cpp` file, and you want to test, you will need `make` so your 
     changes will be built into the library and the header files. 
     
     `make` is a utility for easily building multiple source codes. It use a file named `Makefile` that contains the recipes for how to compile and link the files using compiler and linker. `cmake` is the utility for creating that `Makefile` in a much more easier and portable way than handwriting. It processes a file called `CMakeLists.txt` to create the `Makefile` needed for `make`. 
    
 ## Experiment and testing 
   1. The first test program, [runtime/test/rex_kmpapi/parallel.c](runtime/test/rex_kmpapi/parallel.c) and a simple `Makefile` located in the same folder can be used for testing the implementation of `rex_parallel/single/master/barrier` and others implemented in `rex_kmp.cpp`. Assuming the current session is in the `build` folder following the instruction above. The absolute path for your `llvm-openmp` working directory is `/home/yanyh/llvm-openmp`, so the absolute path for the current folder is `/home/yanyh/llvm-openmp/build`
   
           cd ../runtime/test/rex_kmpapi
           make # create the parallel executable
           export LD_LIBRARY_PATH=/home/yanyh/llvm-openmp/build/runtime/src:$LD_LIBRARY_PATH
           ldd parallel  # to check whether parallel executable will use the library (libomp.so) you just built before. 
           ./parallel
   
   The `export LD_LIBRARY_PATH=...` command setups the library path for the executable by letting LD_LIBRARY_PATH environment variable to include the lib path, which is `/home/yanyh/llvm-openmp/build/runtime/src`. 
     
   Check the `Makefile` to see how the flags are set for compiling the `parallel.c` file. If you need to use the library and the header file for your development, you need to provide the header path and lib path to the `-I` and `-L` flags of the compiler.
     
   1. Please write other test files for other OpenMP functions

## Implementation
  1. All the implementation should be done in [runtime/src/rex_kmp.h](runtime/src/rex_kmp.h) and [runtime/src/rex_kmp.cpp](runtime/src/rex_kmp.cpp) files. You will need mostly refer to the API for KMP compiler support, which is [runtime/src/kmp_csupport.cpp](runtime/src/kmp_csupport.cpp), and others including the support for tasking. 
  
| Features               | C           | C++                     |
|------------------------|-------------|-------------------------|
| parallel/single/master |             | TBB/Kokkos/AMP/RAJA     |
| worksharing            |             | TBB/Kokkos/AMP/RAJA     |
| tasking                | starPU, etc | std::async, std::future |
| accelerator            |             |                         |
| affinity               |             |                         |
| simd                   | cilkplus    |                         |
| concurrent             |             |                         |
| data                   |             | Kokkos                  |

### Header and source files
| Features               | C                    |            C++              |  
|------------------------|----------------------|-----------------------------|
| parallel/single/master |  rex_paralle_c.h/c   |    rex_parallel_cxx.h/cpp   |
| worksharing            |  rex_for_c.h/c       |    rex_for_cxx.h/cpp        |
| tasking                |  rex_task_c.h/c      |    rex_task_cxx.h/cpp       |
  
  
## Implementation Tasks:
For C++ implementation, we should try and experiment the most recent standard features or features in development. [C++11](https://en.wikipedia.org/wiki/C%2B%2B11) and [BOOST](http://www.boost.org/doc/libs/). 
1. C++ interface and implementation for rex::parallel, single, master, barrier, get_thread_num, get_num_threads. We need to consider to use those recent and advanced features of C++ including [varadic template](https://eli.thegreenplace.net/2014/variadic-templates-in-c/) and lambda (Madushan), reference: C++11 thread/join
1. C interface for worksharing, e.g. rex_parallel_for ( ... ) (Kewei), reference: TBB/Kokkos/AMP_PPL parallel_for
1. C++ interface for worksharing (Kewei)
1. C/C++ interface for tasking, reference: std::async and std::future and other tasking runtime

## Resources and References
  1. [Reference manual for Intel OpenMP runtime library](https://www.openmprtl.org/sites/default/files/resources/libomp_20160808_manual.pdf). From the same website for Intel OpenMP runtime library, [https://www.openmprtl.org](https://www.openmprtl.org), you can find more information. The PDF file on the web is not up to date
  with the latest development, and you can build that PDF file or HTML from the source tree. More info can be found from [www](www) folder. 
  1. A ROSE-based OpenMP 3.0 Research Compiler Supporting [Multiple Runtime Libraries (XOMP)](http://rosecompiler.org/ROSE_ResearchPapers/2010-06-AROSEBasedOpenMP3.0ResearchCompiler-IWOMP.pdf)
  1. Kokkos from Sandia: [github](https://github.com/kokkos), Citation: [Kokkos: Enabling manycore performance portability through polymorphic memory access patterns](http://www.sciencedirect.com/science/article/pii/S0743731514001257), [Kokkos Programming Guide](https://fossies.org/linux/Trilinos-trilinos-release/packages/kokkos/doc/Kokkos_PG.pdf], [Kokkos Tutorial](https://github.com/kokkos/kokkos-tutorials)
  1. C++ Multithreading(http://www.cplusplus.com/reference/multithreading/), e.g. [std::thread and std::join](http://www.cplusplus.com/reference/thread/thread/), [std::async and std::future](http://www.cplusplus.com/reference/future/). More information can be found from a book [C++ Concurrency in Action
Practical Multithreading](https://livebook.manning.com/#!/book/c-plus-plus-concurrency-in-action), or [some blogs that are easy to read]
(http://www.bogotobogo.com/cplusplus/C11/1_C11_creating_thread.php).
  1. Threading Building Blocks (TBB)(https://www.threadingbuildingblocks.org/)
  1. [HPX](https://github.com/STEllAR-GROUP/hpx)
  1. [Legion](http://legion.stanford.edu/)
  1. [RAJA](https://github.com/LLNL/RAJA)
