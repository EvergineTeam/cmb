cmake_minimum_required(VERSION 3.21)

# Set the target architecture.
# All modern x86/x64 processors support AVX2.
# Older x86/x64 processors may support SSE2 but not AVX2.
# Very old x86/x64 processors, or non x86/x64
# processors, do not support any of the two.
set(ENABLE_SSE2 True)
set(ENABLE_AVX2 True)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set the project name
project(mesh_booleans)

set(TBB_TEST OFF CACHE BOOL " " FORCE)
set(TBB_EXAMPLES OFF CACHE BOOL " " FORCE)
add_subdirectory(arrangements/external/oneTBB)

set(cinolib_DIR ${PROJECT_SOURCE_DIR}/arrangements/external/Cinolib)
set(CINOLIB_USES_OPENGL_GLFW_IMGUI ON)
set(CINOLIB_USES_SHEWCHUK_PREDICATES ON)

find_package(cinolib REQUIRED)

add_library(cmb SHARED
    code/cmb.h code/cmb.cpp
	code/booleans.h code/booleans.inl
	code/foctree.h code/foctree.inl
)
target_include_directories(cmb PUBLIC
    code/
    arrangements/code/
    arrangements/external/Indirect_Predicates/include/
	${PROJECT_SOURCE_DIR}/arrangements/external/abseil-cpp/
	${PROJECT_SOURCE_DIR}/arrangements/external/oneTBB/
)
target_link_libraries(cmb cinolib tbb)
target_compile_definitions(cmb PUBLIC TBB_PARALLEL=1)

# add the executable
add_executable(cmdline main.cpp)
target_link_libraries(cmdline cmb)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # grant IEEE 754 compliance
        target_compile_options(cmb PUBLIC "/fp:strict")
        # use intrinsic functions
        target_compile_options(cmb PUBLIC "/Oi")
        # set target architecture
        if(ENABLE_AVX2)
                target_compile_options(cmb PUBLIC "/arch:AVX2")
        elseif(ENABLE_SSE2)
                target_compile_options(cmb PUBLIC "/arch:SSE2")
        endif()
        # reserve enough stack size
        target_link_options(cmb PUBLIC "/STACK:8421376")
        # turn off annoying warnings
        target_compile_options(cmb PUBLIC "/D _CRT_SECURE_NO_WARNINGS")
else()
        # set standard optimization level
        target_compile_options(cmb PUBLIC -O2)
        # reserve enough stack size
        target_compile_options(cmb PUBLIC -Wl,-z,stacksize=8421376)
        # grant IEEE 754 compliance
        target_compile_options(cmb PUBLIC -frounding-math)
        # set target architecture
        if(ENABLE_AVX2)
                target_compile_options(cmb PUBLIC "-mavx2")
        elseif(ENABLE_SSE2)
                target_compile_options(cmb PUBLIC "-msse2")
        endif()
endif()
