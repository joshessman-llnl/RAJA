#!/usr/bin/env python

#
# Copyright (c) 2016, Lawrence Livermore National Security, LLC.
#
# Produced at the Lawrence Livermore National Laboratory.
#
# All rights reserved.
#
# For release details and restrictions, please see raja/README-license.txt
#


import sys
from itertools import permutations
from lperm import *


def writeUserIface(ndims):
  
  dim_names = getDimNames(ndims)
  
  
  
  args = map(lambda a: "typename Idx%s"%a.upper(), dim_names)
  idxstr = ", ".join(args)
  args = map(lambda a: "typename T"+a.upper(), dim_names)
  argstr = ", ".join(args)
  print "template<typename POLICY, %s, %s, typename R, typename BODY>" % (idxstr, argstr)
  
  args = map(lambda a: "T%s const &is_%s"%(a.upper(), a), dim_names)
  argstr = ", ".join(args)
  args = map(lambda a: "Idx%s"%(a.upper()), dim_names)  
  idxstr = ", ".join(args)
  print "RAJA_INLINE\nvoid forallN_expanded(%s, BODY const &body, R (BODY::*mf)(%s) const){" % (argstr, idxstr)
  
  args = map(lambda a: "T"+a.upper(), dim_names)
  argstr = ", ".join(args)
  
  args = map(lambda a: "is_"+a, dim_names)
  argstr2 = ", ".join(args)
  print "  // extract next policy"
  print "  typedef typename POLICY::NextPolicy             NextPolicy;"
  print "  typedef typename POLICY::NextPolicy::PolicyTag  NextPolicyTag;"
  print ""
  print "  // extract each loop's execution policy"
  print "  using ExecPolicies = typename POLICY::ExecPolicies;"
  for i in range(0,ndims):
    d = dim_names[i]
    print "  using Policy%s = typename std::tuple_element<%d, typename ExecPolicies::tuple>::type;" % (d.upper(), i)

    
  args = map(lambda a: "Idx%s"%(a.upper()), dim_names)
  argstr = ", ".join(args)  
  print """  
  // Create index type conversion layer
  typedef ForallN_IndexTypeConverter<BODY, %s> IDX_CONV;
  IDX_CONV lamb(body);
""" % argstr


  print "  // call policy layer with next policy"

  args = map(lambda a: "Policy"+a.upper(), dim_names)
  polstr =  ", ".join(args)
  args = map(lambda a: "is_"+a, dim_names)
  isetstr = ", ".join(args)
  
  outstr = "  forallN_policy<NextPolicy, IDX_CONV>(NextPolicyTag(), lamb"
  for d in dim_names:
    outstr += ",\n    ForallN_PolicyPair<Policy%s, T%s>(is_%s)" % (d.upper(), d.upper(), d)
  print outstr + ");"
  
  print "}"
  print ""
  
  print "/*!"
  print " * \\brief Provides abstraction of a %d-nested loop" % (ndims)
  print " *"
  print " * Provides index typing, and initial nested policy unwrapping"
  print " */"
  args = map(lambda a: "typename T"+a.upper(), dim_names)
  templatestr = ", ".join(args)
  args = map(lambda a: "T%s const &is_%s"%(a.upper(), a), dim_names)
  paramstr = ", ".join(args)
  args = map(lambda a: "is_"+a, dim_names)
  argstr = ", ".join(args)
  print """template<typename POLICY, %s, typename BODY>
RAJA_INLINE 
void forallN(%s, BODY body){
  forallN_expanded<POLICY>(%s, body, &BODY::operator());
}""" % (templatestr, paramstr, argstr) 
  print ""



def main(ndims):
  # ACTUAL SCRIPT ENTRY:
  print """//AUTOGENERATED BY gen_forallN_generic.py
/*
 * Copyright (c) 2016, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory.
 *
 * All rights reserved.
 *
 * For release details and restrictions, please see raja/README-license.txt
 */
  
#ifndef RAJA_forallN_generic_HXX__
#define RAJA_forallN_generic_HXX__

#include "forallN_generic_lf.hxx"

namespace RAJA {

""" 
  ndims_list = range(1,ndims+1)
  
  # Create user API call
  for n in ndims_list:
    writeUserIface(n)


  print """

} // namespace RAJA
  
#endif
"""

if __name__ == '__main__':
  main(int(sys.argv[1]))
  
