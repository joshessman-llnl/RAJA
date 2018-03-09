/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file for nested statement collapse struct.
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


#ifndef RAJA_pattern_nested_Collapse_HPP
#define RAJA_pattern_nested_Collapse_HPP

namespace RAJA
{

namespace nested
{


template <typename ExecPolicy, typename ForList, typename... EnclosedStmts>
struct Collapse : public internal::ForList, public internal::CollapseBase,
                  public internal::Statement<ExecPolicy, EnclosedStmts...> {

};





}  // end namespace nested
}  // end namespace RAJA



#endif /* RAJA_pattern_nested_HPP */
