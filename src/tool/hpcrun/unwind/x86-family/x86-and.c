// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2022, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *

//************************* System Include Files ****************************

//*************************** User Include Files ****************************

#include "x86-decoder.h"
#include "x86-interval-arg.h"

#include <lib/isa-lean/x86/instruction-set.h>

//***************************************************************************

unwind_interval *
process_and(xed_decoded_inst_t *xptr, const xed_inst_t *xi, interval_arg_t *iarg)
{
  unwind_interval *next = iarg->current;
  const xed_operand_t* op0 = xed_inst_operand(xi,0);
  xed_operand_enum_t   op0_name = xed_operand_name(op0);

  if (op0_name == XED_OPERAND_REG0) {
    xed_reg_enum_t reg0 = xed_decoded_inst_get_reg(xptr, op0_name);
    if (x86_isReg_SP(reg0)) { 
      x86recipe_t *xr = UWI_RECIPE(iarg->current);
      if (xr->reg.bp_status != BP_UNCHANGED) {
	//----------------------------------------------------------------------
	// we are adjusting the stack pointer via 'and' instruction
	//----------------------------------------------------------------------
	next = new_ui(nextInsn(iarg, xptr), RA_BP_FRAME, &xr->reg);
      } else {
	// remember that SP was adjusted by masking bits
	iarg->sp_realigned = true; 
      }
    }
  }
  return next;
}
