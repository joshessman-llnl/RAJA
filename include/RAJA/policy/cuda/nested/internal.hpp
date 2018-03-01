/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file containing constructs used to run forallN
 *          traversals on GPU with CUDA.
 *
 ******************************************************************************
 */

#ifndef RAJA_policy_cuda_nested_internal_HPP
#define RAJA_policy_cuda_nested_internal_HPP

#include "RAJA/config.hpp"
#include "camp/camp.hpp"
#include "RAJA/pattern/nested.hpp"

#if defined(RAJA_ENABLE_CUDA)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For additional details, please also read RAJA/LICENSE.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the disclaimer below.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the disclaimer (as noted below) in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of the LLNS/LLNL nor the names of its contributors may
//   be used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
// LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include <cassert>
#include <climits>

#include "RAJA/config.hpp"
#include "RAJA/util/defines.hpp"
#include "RAJA/util/types.hpp"

#include "RAJA/pattern/nested/For.hpp"
#include "RAJA/pattern/nested/Lambda.hpp"

#include "RAJA/policy/cuda/MemUtils_CUDA.hpp"
#include "RAJA/policy/cuda/policy.hpp"

#include "RAJA/internal/ForallNPolicy.hpp"
#include "RAJA/internal/LegacyCompatibility.hpp"


namespace RAJA
{


/*!
 * Policy for For<>, executes loop iteration by distributing them over threads.
 * This does no (additional) work-sharing between thread blocks.
 */

struct cuda_thread_exec{};




/*!
 * Policy for For<>, executes loop iteration by distributing iterations
 * exclusively over blocks.
 */

struct cuda_block_exec{};




/*!
 * Policy for For<>, executes loop iteration by distributing work over
 * physical blocks and executing sequentially within blocks.
 */

template<size_t num_blocks>
struct cuda_block_seq_exec{};





/*!
 * Policy for For<>, executes loop iteration by distributing them over threads
 * and blocks, but limiting the number of threads to num_threads.
 */
template<size_t num_threads>
struct cuda_threadblock_exec{};


namespace nested
{


namespace internal
{


struct LaunchDim {

  int blocks;
  int threads;

  RAJA_INLINE
  RAJA_HOST_DEVICE
  constexpr
  LaunchDim() : blocks(1), threads(1) {}


  RAJA_INLINE
  RAJA_HOST_DEVICE
  constexpr
  LaunchDim(int b, int t) : blocks(b), threads(t){}


  RAJA_INLINE
  RAJA_HOST_DEVICE
  constexpr
  LaunchDim(LaunchDim const &c) : blocks(c.blocks), threads(c.threads) {}



  RAJA_INLINE
  RAJA_HOST_DEVICE
  constexpr
  LaunchDim maximum(LaunchDim const & c) const {
    return LaunchDim{
      blocks > c.blocks ? blocks : c.blocks,
      threads > c.threads ? threads : c.threads};
  }

  RAJA_INLINE
  RAJA_HOST_DEVICE
  constexpr
  LaunchDim operator*(LaunchDim const & c) const {
    return LaunchDim{blocks*c.blocks, threads*c.threads};
  }

};

struct CudaLaunchLimits {
  LaunchDim max_dims;
  LaunchDim physical_dims;
};




template<camp::idx_t ArgumentId, typename ExecPolicy>
struct CudaIndexCalc_Policy;




template<camp::idx_t ArgumentId>
struct CudaIndexCalc_Policy<ArgumentId, seq_exec> {
	int i;
	
	template<typename Data>
	RAJA_INLINE
	RAJA_DEVICE
	int assignBegin(Data &data, int carry){
    data.template assign_offset<ArgumentId>(0);
    i = 0;
		return carry;
	}
	
	template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  int increment(Data &data, int carry){
		++ i;

		int len = segment_length<ArgumentId>(data);
		if(i >= len){
			i = 0;
		}
		else{
			carry = 0;
		}
    
    data.template assign_offset<ArgumentId>(i);
		
		return carry;
	}



};




template<camp::idx_t ArgumentId>
struct CudaIndexCalc_Policy<ArgumentId, cuda_thread_exec> {

	int i;

  
	template<typename Data>
	RAJA_INLINE
	RAJA_DEVICE
	int assignBegin(Data &data, int carry){
		i = 0;
		return increment(data, carry);
	}
	
	template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  int increment(Data &data, int carry_in){
		int len = segment_length<ArgumentId>(data);
		i += carry_in;

		int carry_out = i / len;
		i = i - carry_out*len;  // i % len

    data.template assign_offset<ArgumentId>(i);

		return carry_out;
	}



};




template<camp::idx_t Idx>
struct AssignBegin{
	template<typename Data, typename CalcList>
	static
	RAJA_INLINE
	RAJA_DEVICE
	int assign(Data &data, CalcList &calc_list, int carry_in){
		int carry_next = camp::get<Idx>(calc_list).assignBegin(data, carry_in);
		return AssignBegin<Idx-1>::assign(data, calc_list, carry_next);
		//return carry_next == 0 ? 0 : AssignBegin<Idx-1>::assign(data, calc_list, carry_next);
	}
};

template<>
struct AssignBegin<-1>{
	template<typename Data, typename CalcList>
	static
	RAJA_INLINE
	RAJA_DEVICE
	int assign(Data &, CalcList &, int carry){
		return carry;
	}
};


template<camp::idx_t Idx>
struct Increment{
	template<typename Data, typename CalcList>
	static
	RAJA_INLINE
	RAJA_DEVICE
	int increment(Data &data, CalcList &calc_list, int carry_in){
		int carry_next = camp::get<Idx>(calc_list).increment(data, carry_in);
		return carry_next == 0 ? 0 : Increment<Idx-1>::increment(data, calc_list, carry_next);
	}
};

template<>
struct Increment<-1>{
	template<typename Data, typename CalcList>
	static
	RAJA_INLINE
	RAJA_DEVICE
	int increment(Data &, CalcList &, int carry){
		return carry;
	}
};


template<typename SegmentTuple, typename ExecPolicies, typename RangeList>
struct CudaIndexCalc;


template<typename SegmentTuple, typename ... IndexPolicies, camp::idx_t ... RangeInts>
struct CudaIndexCalc<SegmentTuple, camp::list<IndexPolicies...>, camp::idx_seq<RangeInts...>>{

  using CalcList = camp::tuple<IndexPolicies...>;

  CalcList calc_list;





	/**
	 * Assigns beginning index for all arguments in the calc list
	 */


  template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  bool assignBegin(Data &data, int carry){
		return AssignBegin<sizeof...(RangeInts)-1>::assign(data, calc_list, carry) > 0;
	}


	/**
	 * Increment calc list by the increment amount
	 */
  template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  bool increment(Data &data, int carry){
		return Increment<sizeof...(RangeInts)-1>::increment(data, calc_list, carry) > 0;
	}

};


/**
 * Empty calculator case.  Needed for SetShmemWindow when no For loops have
 * been issues (ie. just Tile loops)
 */
template<typename SegmentTuple>
struct CudaIndexCalc<SegmentTuple, camp::list<>, camp::idx_seq<>>{

  RAJA_INLINE
  RAJA_HOST_DEVICE
  CudaIndexCalc()
  {
  }

  
	template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  bool assignBegin(Data &, int){
		// each physical thread will execute
		return false;
	}

	template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  bool increment(Data &, int ){
		// only one execution per physical thread
		return true;
	}


  template<typename Data>
  RAJA_INLINE
  RAJA_DEVICE
  bool assignIndices(Data &, int , int ){
    return false;
  }

};

template<typename SegmentTuple>
using CudaIndexCalc_Terminator = CudaIndexCalc<SegmentTuple, camp::list<>, camp::idx_seq<>>;


template<typename IndexCalcBase, typename CalcN>
struct CudaIndexCalc_Extender;


template<typename SegmentTuple, typename ... CalcPolicies, camp::idx_t ... RangeInts, typename CalcN>
struct CudaIndexCalc_Extender<CudaIndexCalc<SegmentTuple, camp::list<CalcPolicies...>, camp::idx_seq<RangeInts...>>, CalcN>{
  using type = CudaIndexCalc<SegmentTuple, camp::list<CalcPolicies..., CalcN>, camp::idx_seq<RangeInts..., sizeof...(RangeInts)>>;
};

template<typename IndexCalcBase, typename CalcN>
using ExtendCudaIndexCalc = typename CudaIndexCalc_Extender<IndexCalcBase, CalcN>::type;



template<camp::idx_t cur_stmt, camp::idx_t num_stmts>
struct CudaStatementListExecutorHelper{

  using next_helper_t = CudaStatementListExecutorHelper<cur_stmt+1, num_stmts>;

  template<typename StmtTuple, typename Data>
  inline
  static
  __device__
  void exec(StmtTuple &stmts, Data &data, int num_logical_blocks, int block_carry){
    // Execute stmt
    camp::get<cur_stmt>(stmts).exec(data, num_logical_blocks, block_carry);

    // Execute next stmt
    next_helper_t::exec(stmts, data, num_logical_blocks, block_carry);
  }

  template<typename StmtTuple, typename Data>
  inline
  static
  __device__
  void initBlocks(StmtTuple &stmts, Data &data, int num_logical_blocks, int block_stride){
    // Execute stmt
    camp::get<cur_stmt>(stmts).initBlocks(data, num_logical_blocks, block_stride);

    // Execute next stmt
    next_helper_t::initBlocks(stmts, data, num_logical_blocks, block_stride);
  }

  template<typename StmtTuple, typename Data>
  inline
  static
  LaunchDim calculateDimensions(StmtTuple &stmts, Data &data, LaunchDim const &max_physical){
    // Compute this statements launch dimensions
    LaunchDim statement_dims = camp::get<cur_stmt>(stmts).calculateDimensions(data, max_physical);

    // call the next statement in the list
    LaunchDim next_dims = next_helper_t::calculateDimensions(stmts, data, max_physical);

    // Return the maximum of the two
    return statement_dims.maximum(next_dims);
  }

};

template<camp::idx_t num_stmts>
struct CudaStatementListExecutorHelper<num_stmts, num_stmts>{

  template<typename StmtTuple, typename Data>
  inline
  static
  __device__
  void exec(StmtTuple &, Data &, int , int ){
    // nop terminator
  }

  template<typename StmtTuple, typename Data>
  inline
  static
  __device__
  void initBlocks(StmtTuple &, Data &, int , int ){
    // nop terminator
  }

  template<typename StmtTuple, typename Data>
  inline
  static
  LaunchDim calculateDimensions(StmtTuple &, Data &, LaunchDim const &){
    return LaunchDim();
  }

};



template <typename Data, typename Policy, typename IndexCalc>
struct CudaStatementExecutor;

template <typename Data, typename StmtList, typename IndexCalc>
struct CudaStatementListExecutor;


template <typename Data, typename ... Stmts, typename IndexCalc>
struct CudaStatementListExecutor<Data, StatementList<Stmts...>, IndexCalc>{

  using enclosed_stmts_t = camp::tuple<CudaStatementExecutor<Data, Stmts, IndexCalc>...>;
  enclosed_stmts_t enclosed_stmts;


  inline
  __device__
  void exec(Data &data, int num_logical_blocks, int block_carry){

    // Execute statements in order with helper class
    CudaStatementListExecutorHelper<0, sizeof...(Stmts)>::exec(enclosed_stmts, data, num_logical_blocks, block_carry);

  }


  inline
  __device__
  void initBlocks(Data &data, int num_logical_blocks, int block_stride){

    // Execute statements in order with helper class
    CudaStatementListExecutorHelper<0, sizeof...(Stmts)>::initBlocks(enclosed_stmts, data, num_logical_blocks, block_stride);

  }


  RAJA_INLINE
  LaunchDim calculateDimensions(Data const &data, LaunchDim const &max_physical){

    // Compute this statements launch dimensions
    return CudaStatementListExecutorHelper<0, sizeof...(Stmts)>::calculateDimensions(enclosed_stmts, data, max_physical);
  }


};





template<typename StmtList, typename Data>
using cuda_statement_list_executor_t = CudaStatementListExecutor<Data, StmtList, CudaIndexCalc_Terminator<typename Data::segment_tuple_t> >;






template<camp::idx_t ArgumentId, int threads_per_block>
struct CudaBlockLoop {

  int num_blocks;
  int block_i;

  template<typename Stmts, typename Data>
  inline
  __device__
  void initBlockLoop(Stmts &enclosed_stmts, Data &data, int len, int num_logical_blocks, int block_stride){
    num_blocks = len / threads_per_block;
    if(num_blocks*threads_per_block < len){
      num_blocks ++;
    }

    block_i = block_stride % num_blocks;

    // initialize enclosed blocks and statements
    enclosed_stmts.initBlocks(data, num_logical_blocks/num_blocks, block_stride/num_blocks);
  }

  template<typename Stmts, typename Data>
  inline
  __device__
  void execBlockDistribute(Stmts &enclosed_stmts, Data &data, int num_logical_blocks, int block_carry){

    // Increment block by carry-in
    block_i += block_carry;

    // Compute carry out
    // TODO: make this more efficient!
    int carry_out = 0;
    while(block_i >= num_blocks){
      ++ carry_out;
      block_i -= num_blocks;
    }

//    // Assign argument index, and slice the segment for shmem
//    if(threads_per_block == 1){
//      auto &segment = camp::get<ArgumentId>(data.segment_tuple);
//      data.template assign_index<ArgumentId>(*(segment.begin()+block_i*threads_per_block));
//      enclosed_stmts.exec(data, num_logical_blocks, carry_out);
//    }
//    else{
      auto &segment = camp::get<ArgumentId>(data.segment_tuple);
      using segment_t = camp::decay<decltype(segment)>;

      // Store original segment for later
      segment_t orig_segment = segment;

      // Slice the segment for thread iteration
      segment = orig_segment.slice(block_i*threads_per_block, threads_per_block);
      data.template assign_offset<ArgumentId>(0);
      //data.shmem_window_start[ArgumentId] = RAJA::convertIndex<int>(*segment.begin());

      // execute enclosed statements
      //enclosed_stmts.exec(data, num_logical_blocks/num_blocks, rem_logical_block);
      enclosed_stmts.exec(data, num_logical_blocks, carry_out);

      // Replace original segment
      segment = orig_segment;
//    }
  }


  template<typename Executor, typename Data>
  inline
  __device__
  void execBlockLoop(Executor &exec, Data &data, int num_logical_blocks, int block_carry){
    // if we are already in a block work sharing region, we just assign
    // this block region
    if(block_carry >= 0){
      execBlockDistribute(exec.enclosed_stmts, data, num_logical_blocks, block_carry);
    }

    // We need to start the logical block grid-stride loop, before we
    // call ourself to distribute work
    else{

      // initialize block indexing for entire sub-tree
      exec.initBlocks(data, num_logical_blocks, blockIdx.x);
      execBlockDistribute(exec.enclosed_stmts, data, num_logical_blocks, 0);

      // set initial logical block to our physical block
      int logical_block = (int)blockIdx.x + gridDim.x;

      while(logical_block < num_logical_blocks){

        // execute block
        execBlockDistribute(exec.enclosed_stmts, data, num_logical_blocks, gridDim.x);

        // grid stride
        logical_block += gridDim.x;

      }
    }

  }

};








}  // namespace internal
}  // namespace nested
}  // namespace RAJA

#endif  // closing endif for RAJA_ENABLE_CUDA guard

#endif  // closing endif for header file include guard
