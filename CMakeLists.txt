cmake_minimum_required(VERSION 3.27 FATAL_ERROR)

set(PROJECT_NAME patolette)
file(READ VERSION.txt PROJECT_VERSION)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release")
endif()

if (DEFINED SKBUILD)
  # If SKBUILD is on, we're building a Python extension module
  set(SKBUILD ON)
endif()

option(OPENBLAS_FORCE "Force the usage of OpenBLAS" OFF)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(OPENBLAS_HOME "Home directory of OpenBLAS" OFF)

if (SKBUILD AND NOT BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON)
endif()

if (OPENBLAS_HOME)
  file(TO_CMAKE_PATH ${OPENBLAS_HOME} OPENBLAS_HOME)
  set(OPENBLAS_LIB_SEARCH_PATHS ${OPENBLAS_HOME}/lib)
  set(OPENBLAS_HEADERS_SEARCH_PATHS ${OPENBLAS_HOME}/include)
else()
  set(OPENBLAS_LIB_SEARCH_PATHS
    /usr/lib
    /usr/local/lib
    /usr/local/opt/openblas/lib
    /opt/homebrew/opt/openblas/lib
    /usr/lib/x86_64-linux-gnu
  )

  set(OPENBLAS_HEADERS_SEARCH_PATHS
    /usr/include
    /usr/include/openblas
    /usr/local/include
    /usr/local/include/openblas
    /usr/local/opt/openblas/include
    /opt/homebrew/opt/openblas/include
    /usr/include/x86_64-linux-gnu
  )
endif()

set(OPENBLAS_HEADERS_NAMES cblas.h lapack.h)

if (BUILD_SHARED_LIBS)
  set(OPENBLAS_LIB_NAMES libopenblas.dylib libopenblas.so)
  set(OPENBLAS_WIN_LIB_NAMES libopenblas.dll)
else()
  set(OPENBLAS_LIB_NAMES libopenblas.a)
  set(OPENBLAS_WIN_LIB_NAMES libopenblas.dll.a)
endif()

set(
  LIB_SOURCES_LIST

  lib/src/patolette.c

  lib/src/array/array.c
  lib/src/array/matrix2D.c
  lib/src/array/matrix3D.c
  lib/src/array/vector.c

  lib/src/color/CIELuv.c

        lib/src/dither/riemersma.c

  lib/src/math/eigen.c
  lib/src/math/pca.c

  lib/src/quantize/cells.c
  lib/src/quantize/cluster.c
  lib/src/quantize/global.c
  lib/src/quantize/local.c
  lib/src/quantize/sort.c

  lib/src/palette/nearest.c
  lib/src/color/rec2020.c
  lib/src/color/sRGB.c
  lib/src/color/ICtCp.c
  lib/src/color/xyz.c
  lib/src/color/eotf.c
  lib/src/palette/refine.c
        lib/src/palette/create.c
)

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
    target_include_directories(${PROJECT_NAME} PRIVATE ${Python_NumPy_INCLUDE_DIRS})
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

set(BLAS_FOUND OFF)

if (APPLE AND NOT OPENBLAS_FORCE)
    # Try to use Apple's Accelerate framework as BLAS implementation
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if(ACCELERATE_FRAMEWORK)
        set(BLAS_FOUND ON)
        message("BLAS Implementation: Apple's Accelerate framework")
        message("BLAS Implementation located at: ${ACCELERATE_FRAMEWORK}")
        target_compile_definitions(${PROJECT_NAME} PRIVATE PATOLETTE_USE_ACCELERATE)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ACCELERATE_NEW_LAPACK)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${ACCELERATE_FRAMEWORK})
    endif()
endif()

if (NOT BLAS_FOUND)
  # Try to link against OPENBLAS

  find_path(OPENBLAS_HEADERS
    NAMES ${OPENBLAS_HEADERS_NAMES}
    PATHS ${OPENBLAS_HEADERS_SEARCH_PATHS}
    NO_DEFAULT_PATH
  )

  find_library(OPENBLAS_LIB
    NAMES ${OPENBLAS_LIB_NAMES}
    PATHS ${OPENBLAS_LIB_SEARCH_PATHS}
    NO_DEFAULT_PATH
  )

  if (NOT OPENBLAS_LIB)
    find_library(OPENBLAS_LIB
      NAMES ${OPENBLAS_WIN_LIB_NAMES}
      PATHS ${OPENBLAS_LIB_SEARCH_PATHS}
    )
  endif()

  if (OPENBLAS_LIB AND OPENBLAS_HEADERS)
    set(BLAS_FOUND ON)
  endif()

  if (BLAS_FOUND)
    message("BLAS Implementation: OpenBLAS")
    message("BLAS Headers located at: ${OPENBLAS_HEADERS}")
    message("BLAS Implementation located at: ${OPENBLAS_LIB}")
    include_directories(${OPENBLAS_HEADERS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENBLAS_LIB})
  else()
    if (NOT OPENBLAS_LIB)
      message("Could not locate OpenBLAS library")
    endif()
    if (NOT OPENBLAS_HEADERS)
      message("Could not locate OpenBLAS headers")
    endif()
    message(FATAL_ERROR "BLAS implementation not found")
  endif()

  find_package(OpenMP REQUIRED)
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_C)
endif()

# Try to link against FLANN
find_package(FLANN REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE flann::flann)
target_include_directories(${PROJECT_NAME} PRIVATE ${FLANN_INCLUDE_DIRS})

set(FAISS_ENABLE_C_API ON)
set(FAISS_ENABLE_GPU OFF)
set(FAISS_ENABLE_PYTHON OFF)
set(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lib/faiss)
target_include_directories(${PROJECT_NAME} PRIVATE lib)
target_link_libraries(${PROJECT_NAME} PRIVATE faiss_c)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)