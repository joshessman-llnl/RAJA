###############################################################################
# Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
# and other RAJA project contributors. See the RAJA/COPYRIGHT file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

if (ENABLE_OPENMP)
  if(OPENMP_FOUND)
    list(APPEND RAJA_EXTRA_NVCC_FLAGS -Xcompiler ${OpenMP_CXX_FLAGS})
    message(STATUS "OpenMP Enabled")
  else()
    message(WARNING "OpenMP NOT FOUND")
    set(ENABLE_OPENMP Off)
  endif()
endif()

if (ENABLE_TBB)
  find_package(TBB)
  if(TBB_FOUND)
    blt_register_library(
      NAME tbb
      INCLUDES ${TBB_INCLUDE_DIRS}
      LIBRARIES ${TBB_LIBRARIES})
    message(STATUS "TBB Enabled")
  else()
    message(WARNING "TBB NOT FOUND")
    set(ENABLE_TBB Off)
  endif()
endif ()

if (ENABLE_CUDA)
  if(ENABLE_EXTERNAL_CUB)
    find_package(CUB)
    if (CUB_FOUND)
      blt_register_library(
        NAME cub
        INCLUDES ${CUB_INCLUDE_DIRS})
    else()
      message(WARNING "External CUB not found, using submodule.")
      set(ENABLE_EXTERNAL_CUB Off)
    endif()
  else()
    message(STATUS "Using RAJA CUB submodule.")
  endif ()
endif ()

if (ENABLE_HIP)
  if(ENABLE_EXTERNAL_ROCPRIM)
    find_package(ROCPRIM)
    if (ROCPRIM_FOUND)
      blt_register_library(
        NAME rocPRIM
        INCLUDES ${ROCPRIM_INCLUDE_DIRS})
    else()
      message(WARNING "External rocPRIM not found, using submodule.")
      set(ENABLE_EXTERNAL_ROCPRIM Off)
     endif()
  else()
    message(STATUS "Using RAJA rocPRIM submodule.")
  endif ()
endif ()
