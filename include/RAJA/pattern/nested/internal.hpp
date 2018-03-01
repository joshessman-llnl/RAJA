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

#ifndef RAJA_pattern_nested_internal_HPP
#define RAJA_pattern_nested_internal_HPP

#include "RAJA/config.hpp"
#include "RAJA/index/IndexSet.hpp"
#include "RAJA/internal/LegacyCompatibility.hpp"
#include "RAJA/util/defines.hpp"
#include "RAJA/util/types.hpp"

#include "camp/camp.hpp"
#include "camp/concepts.hpp"

#include <type_traits>

namespace RAJA
{
namespace nested
{
namespace internal
{


template <typename... Stmts>
using StatementList = camp::list<Stmts...>;


template <typename ExecPolicy, typename... EnclosedStmts>
struct Statement {
  using enclosed_statements_t = StatementList<EnclosedStmts...>;
  using execution_policy_t = ExecPolicy;
};


template <typename T>
using remove_all_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;

// Universal base of all For wrappers for type traits
struct ForList {
};
struct ForBase {
};
struct CollapseBase {
};
template <camp::idx_t ArgumentId, typename Policy>
struct ForTraitBase : public ForBase {
  constexpr static camp::idx_t index_val = ArgumentId;
  using index = camp::num<ArgumentId>;
  using index_type = camp::nil;  // default to invalid type
  using policy_type = Policy;
  using type = ForTraitBase;  // make camp::value compatible
};





template <typename Iterator>
struct iterable_difftype_getter {
  using type = typename Iterator::iterator::difference_type;
};

template <typename Segments>
using difftype_list_from_segments =
    typename camp::transform<iterable_difftype_getter, Segments>::type;


template <typename Segments>
using difftype_tuple_from_segments =
    typename camp::apply_l<camp::lambda<camp::tuple>,
    difftype_list_from_segments<Segments> >::type;




template <typename Iterator>
struct iterable_value_type_getter {
  using type = typename Iterator::iterator::value_type;
};

template <typename Segments>
using value_type_list_from_segments =
    typename camp::transform<iterable_value_type_getter, Segments>::type;


template <typename Segments>
using index_tuple_from_segments =
    typename camp::apply_l<camp::lambda<camp::tuple>,
                           value_type_list_from_segments<Segments> >::type;




template <typename Policy>
struct StatementExecutor{};


template <typename PolicyType, typename SegmentTuple,typename ... Bodies>
struct LoopData {

  using Self = LoopData<PolicyType, SegmentTuple, Bodies...>;

  using offset_tuple_t = difftype_tuple_from_segments<typename SegmentTuple::TList>;

  using index_tuple_t = index_tuple_from_segments<typename SegmentTuple::TList>;

  using policy_t = PolicyType;

  using segment_tuple_t = camp::decay<SegmentTuple>;
  SegmentTuple segment_tuple;

  using BodiesTuple = camp::tuple<Bodies...> ;
  const BodiesTuple bodies;
  offset_tuple_t offset_tuple;

  int shmem_window_start[segment_tuple_t::TList::size];

  RAJA_INLINE
  LoopData(SegmentTuple const &s, Bodies const & ... b)
      : segment_tuple{s}, bodies{b...}
  {
    for(size_t i = 0;i < segment_tuple_t::TList::size; ++ i){
      shmem_window_start[i] = 0;
    }
    assign_begin_all();
  }

  template <typename PolicyType0, typename SegmentTuple0, typename ... Bodies0>
  RAJA_INLINE
  RAJA_HOST_DEVICE
  constexpr
  LoopData(LoopData<PolicyType0, SegmentTuple0, Bodies0...> &c)
      : segment_tuple{c.segment_tuple}, bodies{c.bodies}, offset_tuple{c.offset_tuple}
  {
  }

  template <camp::idx_t Idx, typename IndexT>
  RAJA_HOST_DEVICE
  RAJA_INLINE
  void assign_offset(IndexT const &i)
  {
    camp::get<Idx>(offset_tuple) = i;
  }


  template <camp::idx_t Idx>
  RAJA_HOST_DEVICE
  RAJA_INLINE
  int assign_begin()
  {
    camp::get<Idx>(offset_tuple) = 0;
    return 0;
  }

  template <camp::idx_t ... Idx>
  RAJA_HOST_DEVICE
  RAJA_INLINE
  void assign_begin_all_expanded(camp::idx_seq<Idx...> const &)
  {
    VarOps::ignore_args( assign_begin<Idx>()... );

  }

  RAJA_HOST_DEVICE
  RAJA_INLINE
  void assign_begin_all()
  {
    assign_begin_all_expanded(camp::make_idx_seq_t<offset_tuple_t::TList::size>{});
  }



  template <camp::idx_t ... Idx>
  RAJA_HOST_DEVICE
  RAJA_INLINE
  index_tuple_t get_begin_index_tuple_expanded(camp::idx_seq<Idx...> const &) const
  {
    return camp::make_tuple( (*camp::get<Idx>(segment_tuple).begin())... );
  }

  RAJA_HOST_DEVICE
  RAJA_INLINE
  index_tuple_t get_begin_index_tuple() const
  {
    return get_begin_index_tuple_expanded(camp::make_idx_seq_t<offset_tuple_t::TList::size>{});
  }

};




RAJA_SUPPRESS_HD_WARN
template <camp::idx_t LoopIndex, camp::idx_t ... Idx, typename Data>
RAJA_HOST_DEVICE
RAJA_INLINE
void invoke_lambda_expanded(camp::idx_seq<Idx...> const &, Data &data)
{
  camp::get<LoopIndex>(data.bodies)(
            (camp::get<Idx>(data.segment_tuple).begin()[camp::get<Idx>(data.offset_tuple)] )...
      );
}


template<camp::idx_t LoopIndex, typename Data>
RAJA_INLINE
RAJA_HOST_DEVICE
void invoke_lambda(Data &data){
  invoke_lambda_expanded<LoopIndex>(camp::make_idx_seq_t<Data::offset_tuple_t::TList::size>{}, data);
}

template<camp::idx_t ArgumentId, typename Data>
RAJA_INLINE
RAJA_HOST_DEVICE
auto segment_length(Data const &data) ->
	typename camp::at_v<typename Data::segment_tuple_t::TList, ArgumentId>::iterator::difference_type
{
	return camp::get<ArgumentId>(data.segment_tuple).end() - 
	       camp::get<ArgumentId>(data.segment_tuple).begin();
}


template <camp::idx_t idx, camp::idx_t N, typename StmtList>
struct StatementListExecutor;




template <camp::idx_t statement_index, camp::idx_t num_statements, typename StmtList>
struct StatementListExecutor{

  template<typename Data>
  static
  RAJA_INLINE
  void exec(Data &&data) {

    // Get the statement we're going to execute
    using statement = camp::at_v<StmtList, statement_index>;

    // Execute this statement
    StatementExecutor<statement>::exec(std::forward<Data>(data));

    // call our next statement
    StatementListExecutor<statement_index+1, num_statements, StmtList>::exec(std::forward<Data>(data));
  }
};


/*
 * termination case, a NOP.
 */

template <camp::idx_t num_statements, typename StmtList>
struct StatementListExecutor<num_statements,num_statements,StmtList> {

  template<typename Data>
  static
  RAJA_INLINE
  void exec(Data &&) {}

};




template<typename StmtList, typename Data>
void execute_statement_list(Data && data){
  StatementListExecutor<0, StmtList::size, StmtList>::exec(std::forward<Data>(data));
}

// Gives all GenericWrapper derived types something to enable_if on
// in our thread_privatizer
struct GenericWrapperBase {};

template <typename Data, typename ... EnclosedStmts>
struct GenericWrapper : public GenericWrapperBase {
  using data_t = camp::decay<Data>;

  data_t &data;

  RAJA_INLINE
  constexpr
  explicit GenericWrapper(data_t &d) : data{d} {}

  RAJA_INLINE
  void exec(){
    execute_statement_list<camp::list<EnclosedStmts...>>(data);
  }
};


/*!
 * Convenience object used to create thread-private a LoopData object.
 */
template <typename T>
struct NestedPrivatizer {
  using data_t = typename T::data_t;
  using value_type = camp::decay<T>;
  using reference_type = value_type &;

  data_t privatized_data;
  value_type privatized_wrapper;

  RAJA_INLINE
  constexpr
  NestedPrivatizer(const T &o) : privatized_data{o.data}, privatized_wrapper(privatized_data) {}

  RAJA_INLINE
  reference_type get_priv() { return privatized_wrapper; }
};



/**
 * @brief specialization of internal::thread_privatize for any wrappers derived
 * from GenericWrapper
 */
template <typename T>
constexpr
typename std::enable_if<std::is_base_of<GenericWrapperBase, camp::decay<T>>::value, NestedPrivatizer<T>>::type thread_privatize(T &wrapper)
{
  return NestedPrivatizer<T>{wrapper};
}




template<camp::idx_t ... Seq, typename ... IdxTypes, typename ... Segments>
RAJA_INLINE
RAJA_HOST_DEVICE
void set_shmem_window_to_begin_expanded(camp::idx_seq<Seq...>, camp::tuple<IdxTypes...> &window, camp::tuple<Segments...> const &segment_tuple){
  VarOps::ignore_args(
      (camp::get<Seq>(window) = camp::get<Seq>(segment_tuple).begin()[0])...
      );
}

template<typename ... IdxTypes, typename ... Segments>
RAJA_INLINE
RAJA_HOST_DEVICE
void set_shmem_window_to_begin(camp::tuple<IdxTypes...> &window, camp::tuple<Segments...> const &segment_tuple){
  using loop_idx = typename camp::make_idx_seq<sizeof...(IdxTypes)>::type;

  set_shmem_window_to_begin_expanded(loop_idx{}, window, segment_tuple);
}






}  // end namespace internal
}  // end namespace nested



#ifdef RAJA_ENABLE_CHAI

namespace detail
{


template <typename T>
struct get_statement_platform
{
  static constexpr Platform value =
          get_platform_from_list<typename T::execution_policy_t, typename T::enclosed_statements_t>::value;
};

/*!
 * Specialization to define the platform for an nested::StatementList, and
 * (by alias) a nested::Policy
 *
 * This collects the Platform from each of it's statements, recursing into
 * each of them.
 */
template <typename... Stmts>
struct get_platform<RAJA::nested::internal::StatementList<Stmts...>>{
  static constexpr Platform value =
        VarOps::foldl(max_platform(), get_statement_platform<Stmts>::value...);
};

/*!
 * Specialize for an empty statement list to be undefined
 */
template <>
struct get_platform<RAJA::nested::internal::StatementList<>>{
  static constexpr Platform value = Platform::undefined;
};



}  // end detail namespace

#endif  // RAJA_ENABLE_CHAI


}  // end namespace RAJA


#endif /* RAJA_pattern_nested_internal_HPP */
