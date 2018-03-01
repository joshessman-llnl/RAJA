/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file containing tiling policies and mechanics
 *          for forallN templates.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-18, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For details about use and distribution, please read RAJA/LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_pattern_nested_HPP
#define RAJA_pattern_nested_HPP


#include "RAJA/config.hpp"
#include "RAJA/policy/cuda.hpp"
#include "RAJA/util/defines.hpp"
#include "RAJA/util/types.hpp"

#include "RAJA/pattern/nested/internal.hpp"

#include "RAJA/util/chai_support.hpp"

#include "RAJA/pattern/shared_memory.hpp"

#include "camp/camp.hpp"
#include "camp/concepts.hpp"
#include "camp/tuple.hpp"

#include <iostream>
#include <type_traits>

namespace RAJA
{
namespace nested
{

/*!
 * A RAJA::nested::forall execution policy.
 *
 * This is just a list of nested::forall statements.
 */
template <typename... Stmts>
using Policy = internal::StatementList<Stmts...>;


///
/// Template list of argument indices
///
template <camp::idx_t ... ArgumentId>
struct ArgList{};


template <typename PolicyType, typename SegmentTuple, typename ... Bodies>
RAJA_INLINE void forall(SegmentTuple &&segments, Bodies && ... bodies)
{
  detail::setChaiExecutionSpace<PolicyType>();

  // TODO: test that all policy members model the Executor policy concept
  // TODO: add a static_assert for functors which cannot be invoked with
  //       index_tuple
  // TODO: add assert that all Lambda<i> match supplied loop bodies

  using segment_t = camp::decay<SegmentTuple>;
  //using offset_tuple_t = internal::difftype_tuple_from_segments<typename segment_t::TList>;
  using loop_data_t = internal::LoopData<PolicyType, segment_t, camp::decay<Bodies>...>;



  // Setup a shared memory window tuple
  //using offset_tuple_t = internal::difftype_tuple_from_segments<typename segment_t::TList>;
  using index_tuple_t = typename loop_data_t::index_tuple_t;
  index_tuple_t shmem_window;
//  printf("shmem_window = %p, %d bytes\n", &shmem_window, (int)sizeof(shmem_window));

  // Turn on shared memory setup
  RAJA::detail::startSharedMemorySetup(&shmem_window, sizeof(index_tuple_t));

  // Create the LoopData object, which contains our policy object,
  // our segments, loop bodies, and the tuple of loop indices
  // it is passed through all of the nested::forall mechanics by-referenece,
  // and only copied to provide thread-private instances.
  loop_data_t loop_data(
          std::forward<SegmentTuple>(segments),
          std::forward<Bodies>(bodies)...);

  // Turn off shared memory setup
  RAJA::detail::finishSharedMemorySetup();

  // initialize the shmem tuple to the beginning of each loop iteration
  internal::set_shmem_window_to_begin(shmem_window, loop_data.segment_tuple);

//  printf("SHARED MEMORY USED: %ld bytes\n",
//      (long)RAJA::detail::getSharedMemorySize());

//  printf("sizeof(loop_data)=%ld bytes\n",(long)sizeof(loop_data));


  // Execute!
  internal::execute_statement_list<PolicyType>(loop_data);



  detail::clearChaiExecutionSpace();
}

}  // end namespace nested

}  // end namespace RAJA


#include "RAJA/pattern/nested/Lambda.hpp"
#include "RAJA/pattern/nested/For.hpp"
#include "RAJA/pattern/nested/Tile.hpp"
#include "RAJA/pattern/nested/Collapse.hpp"
#include "RAJA/pattern/nested/ShmemWindow.hpp"


#endif /* RAJA_pattern_nested_HPP */
