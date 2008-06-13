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

// Implementation of xterm terminal emulator SystemC object

// $Id$


#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <iostream>

#include "XtermSC.h"


// Constructor

SC_HAS_PROCESS( XtermSC );

XtermSC::XtermSC( sc_core::sc_module_name  name ) :
  sc_module( name )
{
  // Register the blocking transport method

  xtermPort.register_b_transport( this, &XtermSC::doReadWrite );

  // Start up the terminal

  xtermInit();

}	/* XtermSC() */


// Destructor to kill the xterm process

XtermSC::~XtermSC()
{
  // Kill any child xterm

  if( xtermPid > 0 ) {
    kill( xtermPid, SIGKILL );
    waitpid( xtermPid, NULL, 0 );
  }
}	// ~XtermSC()
  

// The blocking transport routine.

void
XtermSC::doReadWrite( tlm::tlm_generic_payload &payload,
		      sc_core::sc_time         &delayTime )
{
  // Which command?

  switch( payload.get_command() ) {

  case tlm::TLM_READ_COMMAND:
    doRead( payload, delayTime );
    break;

  case tlm::TLM_WRITE_COMMAND:
    doWrite( payload, delayTime );
    break;

  case tlm::TLM_IGNORE_COMMAND:
    std::cerr << "XtermSC: Unexpected TLM_IGNORE_COMMAND" << std::endl;
    break;
  }

}	// doReadWrite()


// Process a read

void
XtermSC::doRead( tlm::tlm_generic_payload &payload,
		 sc_core::sc_time         &delayTime )
{
  int  ch = xtermRead();

  // Got a char, put it in the right place in the payload. Assume there is
  // only a request for a single byte! If the result was error, then set the
  // byte to -1

  unsigned char *mask = payload.get_byte_enable_ptr();
  unsigned char *dat = payload.get_data_ptr();

  for( int i = 0 ; i < 4 ; i++ ) {
    if( TLM_BYTE_ENABLED == mask[i] ) {
      dat[i] = (ch >= 0) ? (unsigned char)(ch & 0xff) : 0;
      break;
    }
  }

  // ch < 0 could be error or no char available.

  payload.set_response_status( (ch >= 0) ?
			       tlm::TLM_OK_RESPONSE :
			       tlm::TLM_GENERIC_ERROR_RESPONSE );
}	// doRead()


void
XtermSC::doWrite( tlm::tlm_generic_payload &payload,
		  sc_core::sc_time         &delayTime )
{
  unsigned char  ch;
  unsigned char *mask = payload.get_byte_enable_ptr();
  unsigned char *dat = payload.get_data_ptr();

  // Find the char. Assume there is only one sent!

  for( int i = 0 ; i < 4 ; i++ ) {
    if( TLM_BYTE_ENABLED == mask[i] ) {
      ch = dat[i];
      break;
    }
  }

  if( xtermWrite( ch ) < 0 ) {
    payload.set_response_status( tlm::TLM_GENERIC_ERROR_RESPONSE );
  }
  else {
    payload.set_response_status( tlm::TLM_OK_RESPONSE );
  }
}	// doWrite()


// Fire up an xTerm as a child process. 0 on success, -1 on failure, with an
// error code in errno

int
XtermSC::xtermInit()
{
  // Set everything to a default state

  ptMaster = -1;
  ptSlave  = -1;
  xtermPid = -1;

  // Get a slave PTY from the master

  ptMaster = open("/dev/ptmx", O_RDWR);

  if( ptMaster < 0 ) {
    return -1;
  }

  if( grantpt( ptMaster ) < 0 ) {
    return -1;
  }

  if( unlockpt( ptMaster ) < 0 ) {
    return -1;
  }

  // Open the slave PTY

  char *ptSlaveName = ptsname( ptMaster );

  if( NULL == ptSlaveName ) {
    return -1;
  }

  ptSlave = open( ptSlaveName, O_RDWR );	// In and out are the same

  if( ptSlave < 0 ) {
    close( ptMaster );
    ptSlave  = -1;
    ptMaster = -1;

    return -1;
  }

  // Turn off terminal echo and cannonical mode

  struct termios  termInfo;

  if( tcgetattr( ptSlave, &termInfo ) < 0 ) {
    close( ptSlave );
    close( ptMaster );
    ptSlave  = -1;
    ptMaster = -1;

    return -1;
  }

  termInfo.c_lflag &= ~ECHO;
  termInfo.c_lflag &= ~ICANON;

  if( tcsetattr( ptSlave, TCSADRAIN, &termInfo ) < 0 ) {
    close( ptSlave );
    close( ptMaster );
    ptSlave  = -1;
    ptMaster = -1;

    return -1;
  }

  // Fork off the xterm

  xtermPid = fork();

  switch( xtermPid ) {

  case -1:

    close( ptSlave );			// Failure!
    close( ptMaster );
    ptSlave  = -1;
    ptMaster = -1;

    return -1;

  case 0:

    launchXterm( ptSlaveName );		// Child opens the terminal
    return -1;				// Return must mean failure!

  default:

    break;				// The parent carries on.
  }

  //The xterm will issue a code, which indicates it is ready, followed by a
  // newline, which we must swallow.

  int   res;
  char  ch;

  do {
    res = read( ptSlave, &ch, 1 );
  } while( (res >= 0) && (ch != '\n') );

  if( res < 0 ) {
    close( ptSlave );			// Failure!
    close( ptMaster );
    ptSlave  = -1;
    ptMaster = -1;

    return -1;
  }

  return 0;

}	// xtermInit()


// Called in the child to launch the xterm. Should never return - just exit.

void
XtermSC::launchXterm( char *slaveName )
{
  char *arg;
  char *fin = &(slaveName[strlen( slaveName ) - 2]);	// Last 2 chars of name

  // Two different naming conventions for PTYs, lead to two different ways of
  // passing the -S parameter to xterm. Work out the length of string needed
  // in each case (max size of %d is sign + 19 chars for 64 bit!)

  if( NULL == strchr(fin, '/' )) {
    arg = new char[2 + 1 + 1 + 20 + 1];
    sprintf( arg, "-S%c%c%d", fin[0], fin[1], ptMaster );
  }
  else {
    char *slaveBase = ::basename( slaveName );

    arg = new char[2 + strlen( slaveBase ) + 1 + 20 + 1];
    sprintf( arg, "-S%s/%d", slaveBase, ptMaster );
  }

  // Set up the arguments (must be a null terminated list)

  char *argv[3];
  
  argv[0] = (char *)( "xterm" );
  argv[1] = arg;
  argv[2] = NULL;

  // Start up xterm. After this nothing should execute (the image is replaced)

  execvp( "xterm", argv );
  exit( 1 );			// Impossible!!!

}	// launchXterm()


// Native read from the xterm

int
XtermSC::xtermRead()
{
  // Have we an xterm?

  if( ptSlave < 0 ) {
    std::cerr << "XtermSC: No channel available for read" << std::endl;
    return -1;
  }

  // Use a non-blocking read!

  fd_set          readFdSet;
  struct timeval  timeOut = { 0, 0 };

  FD_ZERO( &readFdSet );
  FD_SET( ptSlave, &readFdSet );

  int   res = select( ptSlave + 1, &readFdSet, NULL, NULL, &timeOut );
  char  ch;

  switch( res ) {

  case -1:

    std::cerr << "XtermSC: Error on read select" << std::endl;
    return -1;

  case 0:

    return -1;				// No error - just nothing there

  default:

    if( read( ptSlave, &ch, 1 ) != 1 ) {
      std::cerr << "XtermSC: Error on read" << std::endl;
      return -1;
    }
    else {
      return  (int)ch;
    }
  }

}	// xtermRead()


// Native write to the xterm. Return number of chars written or -1 on error.

int
XtermSC::xtermWrite( char  ch )
{
  // Have we an xterm?

  if( ptSlave < 0 ) {
    std::cerr << "XtermSC: No channel available for write" << std::endl;
    return -1;
  }

  // Blocking write should be fine

  if( write( ptSlave, &ch, 1 ) != 1 ) {
    std::cerr << "XtermSC: Error on write" << std::endl;
    return -1;
  }
  else {
    return 0;
  }
}	// xtermWrite()


// EOF
