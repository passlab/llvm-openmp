
# REX Extensions to LLVM OpenMP Runtime
This respository contains REX extensions to OpenMP, including rex_kmp runtime API, REX HPT, HOMP, etc. 
Since LLVM OpenMP runtime is also compatible with Intel commercial compiler, so the runtime also works with icc. 

1. **master** is the main branch that consoliates the development from other branch when they are stable.
1. **rex_kmpapi** is the branch for creating rex_* API for the kmp functionality so users can directly 
  write their OpenMP style code using runtime API, i.e. without the need of compiler.
1. **rex_ompitrop** is the branch for OpenMP interoperability support including omp_set_wait_policy, omp_thread_create/exit/join/attach, 
  and omp_task, etc. 
1. **rex branch** is for REX extensions including HPT support, and HOMP related extensions.

The [runtime/CMakeLists.txt](runtime/CMakeLists.txt) and [runtime/src/CMakeLists.txt](runtime/src/CMakeLists.txt) define and use REX-related cmake variables to configure which files will be built into final library and what macros will be enabled (check [runtime/src/kmp_config.h.cmake](runtime/src/kmp_config.h.cmake) file). 
1. **LIBOMP_REX_KMPAPI_SUPPORT**: enable features and development in rex_kmpapi branch 
1. **LIBOMP_REX_OMPITROP_SUPPORT**: enable features and development in rex_ompitrop branch 
1. **LIBOMP_REX_SUPPORT**: enable features and development in rex branch
1. **LIBOMP_OMPT_SUPPORT**: enable OMPT support if it is available (currently it is from https://github.com/OpenMPToolsInterface/LLVM-openmp, towards_tr4 branch)

## Build the Library and Installation
  1. clone the repo and checkout the branch you need (only need to do once, mostly)
  
           git clone https://github.com/passlab/llvm-openmp 
           cd llvm-openmp
           git remote update
           git checkout -t origin/rex_kmpapi   
           
  1. cmake to create the makefile with the LIBOMP_* setting you want
  
           mkdir BUILD
           cd BUILD
           cmake -G "Unix Makefiles" -DLIBOMP_OMPT_SUPPORT=on ..
           make; make install
           
      For using other compiler (on fornax), CC and CXX should be set for cmake. For example, on fornax as standalone: 
      
           CC=/opt/gcc-5.3.0-install/bin/gcc CXX=/opt/gcc-5.3.0-install/bin/g++ cmake -DLIBOMP_OMPT_SUPPORT=on ..
           
  1. The above instruction do not install the library, so all the headers and libomp.so are located in openmp/BUILD/runtime/src folder. 
     If you provide -DCMAKE_INSTALL_PREFIX=<install_path> in openmp or LLVM installation, header files (omp.h and ompt.h) and libomp.so 
     library are `<install_path>/include` and `<install_path>/lib` if the runtime is installed standalone. 
     If it is installed as part of clang/llvm, the header location is `<install_path>/lib/clang/5.0.0/include`, and libomp.so is from 
     `<install_path>/lib`. 

     In any case, you need to setup the library path for execution by letting LD_LIBRARY_PATH env include the lib path. 
     For development and compiling, you need to provide the header path and lib path to the -I and -L flags of the compiler.
