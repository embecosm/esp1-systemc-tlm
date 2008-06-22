// ----------------------------------------------------------------------------

//                  CONFIDENTIAL AND PROPRIETARY INFORMATION
//                  ========================================

// Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.

// This file contains confidential and proprietary information of Embecosm and
// is protected by copyright, trade secret and other regional, national and
// international laws, and may be embodied in patents issued or pending.

// Receipt or possession of this file does not convey any rights to use,
// reproduce, disclose its contents, or to manufacture, or sell anything it may
// describe.

// Reproduction, disclosure or use without specific written authorization of
// Embecosm is strictly forbidden.

// Reverse engineering is prohibited.

// ----------------------------------------------------------------------------

// Definition of main SystemC wrapper for the OSCI SystemC wrapper project


// $Id$


#ifndef OR1KSIM_EXT_SC__H
#define OR1KSIM_EXT_SC__H

#include "Or1ksimSC.h"


//! Extended SystemC module class wrapping Or1ksim ISS

//! Derived from Or1ksimSC:: to add the Or1ksimExtSC::isLittleEndian() utility
//! to the interface and redefine Or1ksimSC::doTrans(). Everything else
//! comes from the parent module.

class Or1ksimExtSC
: public Or1ksimSC
{
 public:

  // Constructor (will map directly on to parent)

  Or1ksimExtSC( sc_core::sc_module_name  name,
		const char              *configFile,
		const char              *imageFile );

  // Public utility to return the endianism of the model

  bool  isLittleEndian();

 protected:

  // Updated version of the common thread to make the transport calls. This
  // will be further refined in later derived classes to deal with timing.

  virtual void  doTrans( tlm::tlm_generic_payload &trans );

};	/* Or1ksimExtSC() */


#endif	// OR1KSIM_EXT_SC__H


// EOF
