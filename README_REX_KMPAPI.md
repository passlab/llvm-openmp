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
           git checkout -b rex_kmpapi
          
  1. cmake to create the makefile with the LIBOMP_* setting you want
  
           mkdir build
           cd build
           cmake -G "Unix Makefiles" -DLIBOMP_REX_KMPAPI_SUPPORT=on -DCMAKE_INSTALL_PREFIX=<install_path> ..
           make; make install
           
      For using other compiler (on fornax), CC and CXX should be set for cmake. For example, on fornax as standalone: 
      
           CC=/opt/gcc-5.3.0-install/bin/gcc CXX=/opt/gcc-5.3.0-install/bin/g++ cmake -DLIBOMP_REX_KMPAPI_SUPPORT=on -DCMAKE_INSTALL_PREFIX=<install_path> ..
           
     Each time you change the source code, either rex_kmp.h or rex_kmp.cpp file, and you want to test, you will need to do `make; make install` command. 
           
  1. location for header files (omp.h, rex_kmp.h, and ompt.h) and libomp.so library are `<install_path>/include` and
  `<install_path>/lib` if the runtime is installed standalone. If it is installed as part of clang/llvm, the header 
  location is `<install_path>/lib/clang/5.0.0/include`, and the libomp.so is from `<install_path>/lib`. 
  Setup the library path for execution by letting LD_LIBRARY_PATH env include the lib path. 
  For development and compiling, you need to provide the header path and lib path to the -I and -L flags of the compiler.
  
  ## Implementation
  1. All the implementation should be done in [runtime/src/rex_kmp.h] and [runtime/src/rex_kmp.cpp] files. You will need mostly
  refer to the API for KMP compiler support, which is [runtime/src/kmp_csupport.cpp](runtime/src/kmp_csupport.cpp), and others including the support for tasking. 
    
  ## Experiment and testing 
   1. The first test program, [runtime/test/rex_kmpapi/parallel.c](runtime/test/rex_kmpapi/parallel.c) and a simple Makefile located in the same folder can be used for testing the implementation of parallel/single/master/barrier and others. 
   1. Please write other test files for other OpenMP functions

## Resources
  1. [Reference manual for Intel OpenMP runtime library](https://www.openmprtl.org/sites/default/files/resources/libomp_20160808_manual.pdf). From the same website for Intel OpenMP runtime library, [https://www.openmprtl.org](https://www.openmprtl.org), you can find more information. The PDF file on the web is not up to date
  with the latest development, and you can build that PDF file or HTML from the source tree. More info can be found from [www](www) folder. 
  1. A ROSE-based OpenMP 3.0 Research Compiler Supporting [Multiple Runtime Libraries (XOMP)](http://rosecompiler.org/ROSE_ResearchPapers/2010-06-AROSEBasedOpenMP3.0ResearchCompiler-IWOMP.pdf)
