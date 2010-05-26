// TLM 2.0 extension class for JTAG : implementation

// Copyright (C) 2010 Embecosm Limited

// Contributor: Jeremy Bennett <jeremy.bennett@embecosm.com>

// This file is part of the example programs for "Building a Loosely Timed SoC
// Model with OSCI TLM 2.0".

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.           

// --------------------------------------------------------------------------
// This code is commented throughout for use with Doxygen.
// --------------------------------------------------------------------------

#include "JtagExtensionSC.h"


using tlm::tlm_extension_base;


// --------------------------------------------------------------------------
//! Constructor

//! Set up the various fields
// --------------------------------------------------------------------------
JtagExtensionSC::JtagExtensionSC(JtagExtensionSC::AccessType  _type,
				 int                          _bitSize,
				 bool                         _debugEnabled ) :
  type (_type),
  bitSize (_bitSize),
  debugEnabled (_debugEnabled)
{

}	// JtagExtensionSC ()


// --------------------------------------------------------------------------
//! Variant constructor

//! Set up debug to some default value. Could be done using a default
//! parameter, but this gives a shape that could use more complex default
//! algorithms in the future.
// --------------------------------------------------------------------------
JtagExtensionSC::JtagExtensionSC(JtagExtensionSC::AccessType  _type,
				 int                          _bitSize ) :
  type (_type),
  bitSize (_bitSize),
  debugEnabled (false)
{

}	// JtagExtensionSC ()


// --------------------------------------------------------------------------
//! Clone an instance

//! It is mandatory for us to define this function. Placeholder for now
// --------------------------------------------------------------------------
tlm_extension_base *
JtagExtensionSC::clone() const
{
  JtagExtensionSC *jtesc = new JtagExtensionSC (type, bitSize);

  return jtesc;

}	// clone ()


// --------------------------------------------------------------------------
//! Copy our state from another instance

//! It is mandatory for us to define this function. Placeholder for now
// --------------------------------------------------------------------------
void
JtagExtensionSC::copy_from (tlm_extension_base const &ext)
{
  type    = static_cast<JtagExtensionSC const &>(ext).type;
  bitSize = static_cast<JtagExtensionSC const &>(ext).bitSize;

}	// copy_from ()


// --------------------------------------------------------------------------
//! Accessor to get the type

//! @return  The type of the transaction
// --------------------------------------------------------------------------
JtagExtensionSC::AccessType
JtagExtensionSC::getType () const
{
  return  type;

}	// getType ()


// --------------------------------------------------------------------------
//! Accessor to set the type

//! @param[in] _type  The new type of the transaction
// --------------------------------------------------------------------------
void
JtagExtensionSC::setType (JtagExtensionSC::AccessType _type)
{
  type = _type;

}	// setType ()


// --------------------------------------------------------------------------
//! Accessor to get the bit size

//! @return  The bit size of the transaction
// --------------------------------------------------------------------------
int
JtagExtensionSC::getBitSize () const
{
  return  bitSize;

}	// getBitSize ()


// --------------------------------------------------------------------------
//! Accessor to set the bit size

//! @param[in] _bitSize  The new bit size of the transaction
// --------------------------------------------------------------------------
void
JtagExtensionSC::setBitSize (int  _bitSize)
{
  bitSize = _bitSize;

}	// setBitSize ()


// --------------------------------------------------------------------------
//! Accessor to get the debug flag

//! @return  The debug flag of the transaction
// --------------------------------------------------------------------------
bool
JtagExtensionSC::getDebugEnabled () const
{
  return  debugEnabled;

}	// getDebugEnabled ()


// --------------------------------------------------------------------------
//! Accessor to set the debug flag

//! @param[in] _debugEnabled  The new debug flag of the transaction
// --------------------------------------------------------------------------
void
JtagExtensionSC::setDebugEnabled (bool  _debugEnabled)
{
  debugEnabled = _debugEnabled;

}	// setDebugEnabled ()
