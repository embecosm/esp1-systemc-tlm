// ----------------------------------------------------------------------------

// Example Programs for "Building a Loosely Timed SoC Model with OSCI TLM 2.0"

// Copyright (C) 2008  Embecosm Limited

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// ----------------------------------------------------------------------------

// Top level for test of Or1ksim with logger

// $Id$


#include "tlm.h"
#include "Or1ksimSC.h"
#include "LoggerSC.h"


//! Main program building to test the Or1ksim SystemC wrapper

//! Parses arguments, instantiates the modules and connects up the ports. Then
//! runs forever.

int  sc_main( int   argc,
	      char *argv[] )
{
  if( argc != 3 ) {
    fprintf( stderr, "Usage: TestSC <config_file> <image_file>\n" );
    exit( 1 );
  }

  // Instantiate the modules
  Or1ksimSC  iss( "or1ksim", argv[1], argv[2] );
  LoggerSC   logger( "logger" );

  // Connect up the TLM ports
  iss.dataBus( logger.loggerSocket );

  // Run it forever
  sc_core::sc_start();
  return 0;			// Should never happen!

}	// sc_main()


// EOF
