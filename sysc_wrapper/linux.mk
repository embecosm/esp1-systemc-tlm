# ----------------------------------------------------------------------------

#                  CONFIDENTIAL AND PROPRIETARY INFORMATION
#                  ========================================

# Unpublished copyright (c) 2008 Embecosm. All Rights Reserved.

# This file contains confidential and proprietary information of Embecosm and
# is protected by copyright, trade secret and other regional, national and
# international laws, and may be embodied in patents issued or pending.

# Receipt or possession of this file does not convey any rights to use,
# reproduce, disclose its contents, or to manufacture, or sell anything it may
# describe.

# Reproduction, disclosure or use without specific written authorization of
# Embecosm is strictly forbidden.

# Reverse engineering is prohibited.

# ----------------------------------------------------------------------------

# Linux makefile for OSCI SystemC wrapper project main wrapper

# $Id$

# Tools

CXX = g++

CXXFLAGS += -ggdb -DSC_INCLUDE_DYNAMIC_PROCESSES

SYSTEMC_HOME=/opt/systemc_debug

# Where stuff is

OR1KSIM = /opt/or1ksim
OR1KSIMLIB = $(OR1KSIM)/lib

INCDIRS = -I$(OR1KSIM)/include \
	  -I$(SYSTEMC_HOME)/include \
	  -I$(TLM_HOME)/include/tlm

LIBDIRS = -L$(OR1KSIMLIB) \
	  -L$(SYSTEMC_HOME)/lib-$(TARGET_ARCH)

LIBS    = -lsim -lsystemc


# ----------------------------------------------------------------------------
# Make the lot

.PHONY: all
all: TestSC SimpleSocSC SyncSocSC


# ----------------------------------------------------------------------------
# Logger test of the basic ISS

TestSC: TestSC.o Or1ksimSC.o LoggerSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

TestSC.o: TestSC.cpp Or1ksimSC.h LoggerSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimSC.o: Or1ksimSC.cpp Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

LoggerSC.o: LoggerSC.cpp LoggerSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# Simple SoC with the basic ISS

SimpleSocSC: SimpleSocSC.o Or1ksimSC.o Or1ksimExtSC.o UartSC.o TermSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

SimpleSocSC.o: SimpleSocSC.cpp Or1ksimExtSC.h UartSC.h TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimExtSC.o: Or1ksimExtSC.cpp Or1ksimExtSC.h Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartSC.o: UartSC.cpp UartSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

TermSC.o: TermSC.cpp TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# Synchronous SoC with the synchronous ISS

SyncSocSC: SyncSocSC.o Or1ksimSyncSC.o UartSyncSC.o TermSyncSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

SyncSocSC.o: SyncSocSC.cpp Or1ksimSC.h UartSC.h TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimSyncSC.o: Or1ksimSyncSC.cpp Or1ksimSyncSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartSyncSC.o: UartSyncSC.cpp UartSyncSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

TermSyncSC.o: TermSyncSC.cpp TermSyncSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# Documentation

doc:
	doxygen doxygen.config


# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM)    *.o
	$(RM) -r doc
	$(RM)    TestSC
	$(RM)    SimpleSocSC
	$(RM)    SyncSocSC
