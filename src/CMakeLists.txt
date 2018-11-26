add_executable(gol main.c board.c rle.c mpc.c life.c)
target_include_directories(gol PRIVATE ${PROJECT_SOURCE_DIR}/include)
set_property(TARGET gol
             PROPERTY C_STANDARD 11)

#find_package(OpenMP)
#if(OpenMP_C_FOUND)
  #target_link_libraries(gol PRIVATE OpenMP::OpenMP_C)
#endif()

# Compile Options
include(compile-flags-helpers)
include(${PROJECT_SOURCE_DIR}/optimization_flags.cmake)

if (DEFINED ADDITIONAL_BENCHMARK_COMPILE_OPTIONS)
  add_compiler_option_to_target_type(gol Benchmark PRIVATE ${ADDITIONAL_BENCHMARK_COMPILE_OPTIONS})
endif()

foreach(compile_type IN ITEMS Release RelWithDebInfo)
  add_compiler_option_to_target_type(gol ${compile_type} PRIVATE ${ADDITIONAL_RELEASE_COMPILE_OPTIONS})
  add_linker_option_to_target_type(gol ${compile_type} PRIVATE ${ADDITIONAL_RELEASE_LINK_OPTIONS})
endforeach()

add_compiler_option_to_target_type(gol Debug PRIVATE ${ADDITIONAL_DEBUG_COMPILE_OPTIONS})

# Linker Options

if (DEFINED ADDITIONAL_BENCHMARK_LINK_OPTIONS)
  add_linker_option_to_target_type(gol Benchmark PRIVATE ${ADDITIONAL_BENCHMARK_LINK_OPTIONS})
endif()

add_sanitizers_to_target(gol Debug PRIVATE address undefined)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if((result) AND USE_IPO)
  set_property(TARGET gol PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS gol RUNTIME DESTINATION bin)
