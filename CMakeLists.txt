cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(PROJECT_NAME patolette)
file(READ VERSION.txt PROJECT_VERSION)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

option(PATOLETTE_USE_ACCELERATE "On macOS, use Apple's Accelerate framework" OFF)

set(BUILD_SHARED_LIBS ON)

if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

set(
  LIB_SOURCES_LIST

  lib/src/patolette.c

  lib/src/array/array.c
  lib/src/array/matrix2D.c
  lib/src/array/matrix3D.c
  lib/src/array/vector.c

  lib/src/color/CIELuv.c
  lib/src/color/rec2020.c
  lib/src/color/sRGB.c
  lib/src/color/ICtCp.c
  lib/src/color/xyz.c
  lib/src/color/eotf.c

  lib/src/dither/riemersma.c

  lib/src/math/eigen.c
  lib/src/math/pca.c

  lib/src/palette/create.c
  lib/src/palette/nearest.c
  lib/src/palette/refine.c

  lib/src/quantize/cells.c
  lib/src/quantize/cluster.c
  lib/src/quantize/global.c
  lib/src/quantize/local.c
  lib/src/quantize/sort.c
)

if (DEFINED SKBUILD)
  set(SKBUILD ON)
endif()

if (SKBUILD)
  # Building Python extension module

  find_package(
    Python
    COMPONENTS Interpreter Development.Module NumPy
    REQUIRED
  )

  # Compile patolette.pyx into a ${PROJECT_NAME}.c file
  add_custom_command(
    OUTPUT ${PROJECT_NAME}_cython.c
    COMMENT
      "Making ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_cython.c from ${CMAKE_CURRENT_SOURCE_DIR}/patolette.pyx"
    COMMAND Python::Interpreter -m cython
            "${CMAKE_CURRENT_SOURCE_DIR}/src/patolette/patolette.pyx" --output-file ${PROJECT_NAME}_cython.c
            --module-name ${PROJECT_NAME}
    DEPENDS src/patolette/patolette.pyx
    VERBATIM
  )

  python_add_library(
    ${PROJECT_NAME} MODULE 
    ${PROJECT_NAME}_cython.c
    ${LIB_SOURCES_LIST}
    WITH_SOABI
  )

  if (Python_NumPy_FOUND)
    # Include numpy headers for cimports in patolette.pyx
    target_compile_definitions(${PROJECT_NAME} PRIVATE NPY_NO_DEPRECATED_API)
    target_include_directories(${PROJECT_NAME} PRIVATE "${Python_NumPy_INCLUDE_DIRS}")
  else()
    message(FATAL_ERROR "Could not find NumPy headers")
  endif()

  install(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_NAME})
else()
  # Building standalone library

  add_library(
    ${PROJECT_NAME}
    ${LIB_SOURCES_LIST}
  )

  set_target_properties(
    ${PROJECT_NAME} 
    PROPERTIES 
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER lib/include/patolette.h
  )

  install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${PROJECT_NAME}/lib
    ARCHIVE DESTINATION ${PROJECT_NAME}/lib
    PUBLIC_HEADER DESTINATION ${PROJECT_NAME}/include
  )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE lib/include)

set(BLA_VENDOR "OpenBLAS")
if (APPLE)
  if (PATOLETTE_USE_ACCELERATE)
    set(BLA_VENDOR "Apple")
  else()
    # find_package(BLAS) doesn't take care of include
    # directories because typically they're not needed.
    # But we make use of cblas.h and lapack.h from OpenBLAS,
    # so it needs to be handled. For now, the path is hardcoded
    # and assumes a brew installation of OpenBLAS.
    set(OPENBLAS_INCLUDE_DIR "/opt/homebrew/opt/openblas/include")
    if(EXISTS "${OPENBLAS_INCLUDE_DIR}")
      target_include_directories(${PROJECT_NAME} PRIVATE "${OPENBLAS_INCLUDE_DIR}")
    else()
      message(FATAL_ERROR "OpenBLAS include directory not found at ${OPENBLAS_INCLUDE_DIR}")
    endif()
  endif()
endif()

if (UNIX)
  find_package(BLAS REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE BLAS::BLAS)
endif()

if (WIN32)
  # TODO
endif()

find_package(FLANN REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE flann::flann)

set(FAISS_ENABLE_C_API ON)
set(FAISS_ENABLE_GPU OFF)
set(FAISS_ENABLE_PYTHON OFF)
set(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lib/faiss)
target_include_directories(${PROJECT_NAME} PRIVATE lib)
target_link_libraries(${PROJECT_NAME} PRIVATE faiss_c)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)