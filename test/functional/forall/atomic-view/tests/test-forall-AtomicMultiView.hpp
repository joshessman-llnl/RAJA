//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

///
/// Header file containing basic functional tests for atomic operations with forall and views.
///

#ifndef __TEST_FORALL_ATOMIC_MULTIVIEW_HPP__
#define __TEST_FORALL_ATOMIC_MULTIVIEW_HPP__

#include <math.h>

template <typename ExecPolicy,
          typename AtomicPolicy,
          typename WORKINGRES,
          typename IdxType,
          typename T>
void ForallAtomicMultiViewTestImpl( IdxType N )
{
  // Functionally similar to ForallAtomicViewTestImpl

  int halfsqr = (int)(sqrt(N/2)); // dest[] dimension
  int doublehalfsqr = (int)(sqrt(N/2)*2); // source[] dimension

  RAJA::TypedRangeSegment<IdxType> seg(0, N);
  RAJA::TypedRangeSegment<IdxType> seg_hundred(0, halfsqr);
  RAJA::TypedRangeSegment<IdxType> seg_twohundred(0, doublehalfsqr);

  camp::resources::Resource work_res{WORKINGRES()};
  camp::resources::Resource host_res{camp::resources::Host()};

  T * actualsource = work_res.allocate<T>(N);
  T * actualdest = work_res.allocate<T>(N/2);
  T * check_array = host_res.allocate<T>(N/2);

  // assumes each source[] will be 2x size of each dest[]
  T ** source = new T * [doublehalfsqr];
  for ( int ii = 0; ii < doublehalfsqr; ++ii )
  {
    source[ii] = actualsource+(ii*halfsqr);
  }

  // assumes each dest[] will be a square matrix
  T ** dest = new T * [halfsqr];
  for ( int ii = 0; ii < halfsqr; ++ii )
  {
    dest[ii] = actualdest+(ii*halfsqr);
  }

#if defined(RAJA_ENABLE_CUDA)
  cudaErrchk(cudaDeviceSynchronize());
#endif

#if defined(RAJA_ENABLE_HIP)
  hipErrchk(hipDeviceSynchronize());
#endif

  RAJA::forall<RAJA::seq_exec>(seg,
                               [=](IdxType i) { actualsource[i] = (T)1; });

  // use atomic add to reduce the array
  // 1D defaut MultiView
  RAJA::MultiView<T, RAJA::Layout<1>> vec_view(source, N);

  // 1D MultiView with array-of-pointers index in 1st position
  RAJA::MultiView<T, RAJA::Layout<1>, 1> sum_view(dest, N);
  auto sum_atomic_view = RAJA::make_atomic_view<AtomicPolicy>(sum_view);


  // Zero out dest using atomic MultiView
  RAJA::forall<ExecPolicy>(seg_hundred, [=] RAJA_HOST_DEVICE(IdxType i) {
    for ( int aopidx = 0; aopidx < halfsqr; ++aopidx )
    {
      sum_atomic_view(i,aopidx) = (T)0;
    }
  });

  // Assign values to dest using atomic MultiView
  RAJA::forall<ExecPolicy>(seg_hundred, [=] RAJA_HOST_DEVICE(IdxType i) {
    for ( int aopidx = 0; aopidx < doublehalfsqr; ++aopidx )
    {
      sum_atomic_view(i, aopidx/2) += vec_view(aopidx,i);
    }
  });

  work_res.memcpy( check_array, actualdest, sizeof(T) * N/2 );

#if defined(RAJA_ENABLE_CUDA)
  cudaErrchk(cudaDeviceSynchronize());
#endif

#if defined(RAJA_ENABLE_HIP)
  hipErrchk(hipDeviceSynchronize());
#endif

  for (IdxType i = 0; i < N / 2; ++i) {
    EXPECT_EQ((T)2, check_array[i]);
  }

  work_res.deallocate( actualsource );
  work_res.deallocate( actualdest );
  host_res.deallocate( check_array );
}

TYPED_TEST_SUITE_P(ForallAtomicMultiViewTest);
template <typename T>
class ForallAtomicMultiViewTest : public ::testing::Test
{
};

TYPED_TEST_P(ForallAtomicMultiViewTest, AtomicMultiViewForall)
{
  using AExec   = typename camp::at<TypeParam, camp::num<0>>::type;
  using APol    = typename camp::at<TypeParam, camp::num<1>>::type;
  using ResType = typename camp::at<TypeParam, camp::num<2>>::type;
  using IdxType = typename camp::at<TypeParam, camp::num<3>>::type;
  using DType   = typename camp::at<TypeParam, camp::num<4>>::type;

  ForallAtomicMultiViewTestImpl<AExec, APol, ResType, IdxType, DType>( 20000 );
}

REGISTER_TYPED_TEST_SUITE_P(ForallAtomicMultiViewTest,
                            AtomicMultiViewForall);

#endif  //__TEST_FORALL_ATOMIC_MULTIVIEW_HPP__
