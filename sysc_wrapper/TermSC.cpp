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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <iostream>

#include "TermSC.h"


// Constructor

SC_HAS_PROCESS( TermSC );

TermSC::TermSC( sc_core::sc_module_name  name,
		unsigned long int        baudRate ) :
  sc_module( name )
{
  // Set up the two threads

  SC_THREAD( termRxThread );
  SC_THREAD( termTxThread );

  // Calculate the delay. No configurability here - 1 start, 8 data and 1 stop
  // = total 10 bits.

  charDelay = sc_core::sc_time( 10.0 / (double)baudRate, sc_core::SC_SEC );

  // Start up the terminal

  xtermInit();

}	/* TermSC() */


// Destructor to kill the xterm process

TermSC::~TermSC()
{
  killTerm( NULL );		// Get rid of the xterm

}	// ~TermSC()
  

// Sit in a loop getting characters and passing them on after the relevant
// delay.

void
TermSC::termRxThread()
{
  while( true ) {
    unsigned char  ch = (unsigned char)xtermRead();	// Blocking

    wait( charDelay );		// Allow time for the char to get on the line.
    termTx.write( ch );		// Send it

    wait( *ioEvent );		// Wait for some I/O on the terminal
  }

}	// termRxThread()


// Loop writing any character that comes in to the screen

void
TermSC::termTxThread()
{
  while( true ) {
    unsigned char  ch = termRx.read();	// Blocking read from the FIFO

    xtermWrite( ch );			// Write it to the screen
  }
}	// termTxThread()


// Fire up an xTerm as a child process. 0 on success, -1 on failure, with an
// error code in errno

int
TermSC::xtermInit()
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
    killTerm( NULL );
    return -1;
  }

  if( unlockpt( ptMaster ) < 0 ) {
    killTerm( NULL );
    return -1;
  }

  // Open the slave PTY

  char *ptSlaveName = ptsname( ptMaster );

  if( NULL == ptSlaveName ) {
    killTerm( NULL );
    return -1;
  }

  ptSlave = open( ptSlaveName, O_RDWR );	// In and out are the same

  if( ptSlave < 0 ) {
    killTerm( NULL );
    return -1;
  }

  // Turn off terminal echo and cannonical mode

  struct termios  termInfo;

  if( tcgetattr( ptSlave, &termInfo ) < 0 ) {
    killTerm( NULL );
    return -1;
  }

  termInfo.c_lflag &= ~ECHO;
  termInfo.c_lflag &= ~ICANON;

  if( tcsetattr( ptSlave, TCSADRAIN, &termInfo ) < 0 ) {
    killTerm( NULL );
    return -1;
  }

  // Fork off the xterm

  xtermPid = fork();

  switch( xtermPid ) {

  case -1:

    killTerm( NULL );				// Failure
    return -1;

  case 0:

    launchTerm( ptSlaveName );		// Child opens the terminal
    return -1;				// Should be impossible!

  default:

    return  setupTerm();		// The parent carries on.
  }

}	// xtermInit()


// Close any open file descriptors and shut down the Xterm. If the message is
// non-null print it with perror.

void
TermSC::killTerm( const char *mess )
{
  if( -1 != ptSlave ) {			// Close down the slave
    Fd2Inst *prev = NULL;
    Fd2Inst *cur  = instList;

    while( NULL != cur ) {		// Delete this FD from the FD->inst
      if( cur->fd = ptSlave ) {		// mapping if it exists
	if( NULL == prev ) {
	  instList->next = cur->next;
	}
	else {
	  prev->next = cur->next;
	}
	delete cur;
	break;
      }
    }

    close( ptSlave );			// Close the FD
    ptSlave  = -1;
  }

  if( -1 != ptMaster ) {		// Close down the master
    close( ptMaster );
    ptMaster = -1;
  }

  if( xtermPid > 0 ) {			// Kill the terminal
    kill( xtermPid, SIGKILL );
    waitpid( xtermPid, NULL, 0 );
  }

  if( NULL != mess ) {			// If we really want a message
    perror( mess );
  }
}	// killTerm()


// Called in the child to launch the xterm. Should never return - just exit.

void
TermSC::launchTerm( char *slaveName )
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

}	// launchTerm()


// Called in ther parent to set the terminal up. Return -1 on failure, 0 on
// success.

int
TermSC::setupTerm()
{
  int   res;
  char  ch;

  // The xTerm spits out some code, followed by a newline which we swallow up

  do {
    res = read( ptSlave, &ch, 1 );
  } while( (res >= 0) && (ch != '\n') );

  if( res < 0 ) {
    killTerm( NULL );
    return -1;
  }

  // I/O from the terminal will be asynchronous, with SIGIO triggered when
  // anything happens. That way, we can let the SystemC thread yield.

  // Record the mapping of slave FD to this instance

  Fd2Inst *newMap = new Fd2Inst;

  newMap->fd   = ptSlave;
  newMap->inst = this;
  newMap->next = instList;
  instList     = newMap;

  // Dynamically create the SystemC event we will use.

  ioEvent = new sc_core::sc_event();

  // Install a signal handler

  struct sigaction  action;
  action.sa_sigaction = ioHandler;
  action.sa_flags     = SA_SIGINFO;

  if( sigaction( SIGIO, (const struct sigaction *)&action, NULL ) != 0 ) {
    killTerm( "Sigaction Failed" );
    return -1;
  }

  // Make the Slave FD asynchronous with this process.

  if( fcntl( ptSlave, F_SETOWN, getpid()) != 0 ) {
    killTerm( "SETOWN" );
    return -1;
  }

  int flags = fcntl( ptSlave, F_GETFL );

  if( fcntl( ptSlave, F_SETFL, flags | O_ASYNC ) != 0 ) {
    killTerm( "SETFL" );
    return -1;
  }

  return 0;

}	// setupTerm()


// Static pointer to the fd->instance mapping list

Fd2Inst *TermSC::instList = NULL;


// Static I/O handler, since sigaction cannot cope with a member function

void
TermSC::ioHandler( int        signum,
		   siginfo_t *si,
		   void      *p )
{
  // Use select to find which FD atually triggered to ioHandler. According to
  // the iterature, SIGIO ought to set this in the siginfo, but it doesn't.

  fd_set          readFdSet;
  struct timeval  timeout = { 0, 0 };
  int             maxFd   = 0;		// Largest FD we find

  FD_ZERO( &readFdSet );

  for( Fd2Inst *cur = instList; cur != NULL ; cur = cur->next ) {
    FD_SET( cur->fd, &readFdSet );
    maxFd = (cur->fd > maxFd ) ? cur->fd : maxFd;
  }

  // Non-blocking select, in case there is nothing there!!

  switch( select( maxFd + 1, &readFdSet, NULL, NULL, &timeout ) ) {

  case -1:
    perror( "TermSC: Error on handler select" );
    return;

  case 0:
    return;		// Nothing to read - ignore
  }

  // Now trigger the handlers of any FD's that were set

  for( Fd2Inst *cur = instList; cur != NULL ; cur = cur->next ) {
    if( FD_ISSET( cur->fd, &readFdSet )) {
      (cur->inst)->ioEvent->notify();
    }
  }
}	// ioHandler()


// Native read from the xterm. We'll only be calling this if we already know
// there is something to read.

int
TermSC::xtermRead()
{
  // Have we an xterm?

  if( ptSlave < 0 ) {
    std::cerr << "TermSC: No channel available for read" << std::endl;
    return -1;
  }

  // Use a blocking read! No need to select, since that was done in the IO
  // event handler

  char  ch;

  if( read( ptSlave, &ch, 1 ) != 1 ) {
    perror( "TermSC: Error on read" );
    return -1;
  }
  else {
    return  (int)ch;
  }
}	// xtermRead()


// Native write to the xterm. Ignore any errors

void
TermSC::xtermWrite( char  ch )
{
  // Have we an xterm?

  if( ptSlave < 0 ) {
    std::cerr << "TermSC: No channel available for write" << std::endl;
    return;
  }

  // Blocking write should be fine. Note any error!

  if( write( ptSlave, &ch, 1 ) != 1 ) {
    std::cerr << "TermSC: Error on write" << std::endl;
  }
}	// xtermWrite()


// EOF
