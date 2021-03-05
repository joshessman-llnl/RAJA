###############################################################################
# Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
# and other RAJA project contributors. See the RAJA/COPYRIGHT file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

include (FindPackageHandleStandardArgs)

if(DEFINED CUB_DIR)
  list(APPEND CUB_HINTS ${CUB_DIR}/)
  list(APPEND CUB_HINTS ${CUB_DIR}/include)
endif()

if(DEFINED CUDA_TOOLKIT_ROOT_DIR)
  list(APPEND CUB_HINTS ${CUDA_TOOLKIT_ROOT_DIR}/include)
  list(APPEND CUB_HINTS ${CUDA_TOOLKIT_ROOT_DIR}/include/thrust/system/cuda/detail)
endif()

find_path(CUB_INCLUDE_DIRS
  NAMES cub/cub.cuh
  HINTS
    ${CUB_HINTS})

unset(CUB_HINTS)

find_package_handle_standard_args(
  CUB
  DEFAULT_MSG
  CUB_INCLUDE_DIRS)

if (CUB_INCLUDE_DIRS)
  set(CUB_FOUND True)
else ()
  set(CUB_FOUND False)
endif()
