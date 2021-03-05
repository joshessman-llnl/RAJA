###############################################################################
# Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
# and other RAJA project contributors. See the RAJA/COPYRIGHT file for details.
#
# SPDX-License-Identifier: (BSD-3-Clause)
###############################################################################

include (FindPackageHandleStandardArgs)

if(DEFINED ROCPRIM_DIR)
  list(APPEND ROCPRIM_HINTS ${ROCPRIM_DIR}/)
  list(APPEND ROCPRIM_HINTS ${ROCPRIM_DIR}/include)
  list(APPEND ROCPRIM_HINTS ${ROCPRIM_DIR}/rocprim/include)
endif()

if(DEFINED HIP_ROOT_DIR)
  # paths where HIP_ROOT_DIR is in rocm install like /opt/rocm/hip
  list(APPEND ROCPRIM_HINTS ${HIP_ROOT_DIR}/../rocprim)
  list(APPEND ROCPRIM_HINTS ${HIP_ROOT_DIR}/../rocprim/include)
  list(APPEND ROCPRIM_HINTS ${HIP_ROOT_DIR}/../include)
  # paths where HIP_ROOT_DIR is rocm install like /opt/rocm
  list(APPEND ROCPRIM_HINTS ${HIP_ROOT_DIR}/rocprim)
  list(APPEND ROCPRIM_HINTS ${HIP_ROOT_DIR}/rocprim/include)
  list(APPEND ROCPRIM_HINTS ${HIP_ROOT_DIR}/include)
endif()

find_path(ROCPRIM_INCLUDE_DIRS
  NAMES rocprim/rocprim.hpp
  HINTS
    ${ROCPRIM_HINTS})

unset(ROCPRIM_HINTS)

find_package_handle_standard_args(
  ROCPRIM
  DEFAULT_MSG
  ROCPRIM_INCLUDE_DIRS)

if (ROCPRIM_INCLUDE_DIRS)
  set(ROCPRIM_FOUND True)
else ()
  set(ROCPRIM_FOUND False)
endif()
