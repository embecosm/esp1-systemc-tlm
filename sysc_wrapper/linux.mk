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
all: TestSC SimpleSocSC


# ----------------------------------------------------------------------------
# 

TestSC: TestSC.o Or1ksimSC.o LoggerSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

TestSC.o: TestSC.cpp Or1ksimSC.h DataReporterSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimSC.o: Or1ksimSC.cpp Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

LoggerSC.o: LoggerSC.cpp LoggerSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# 

SimpleSocSC: SimpleSocSC.o Or1ksimSC.o UartSC.o TermSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

SimpleSocSC.o: SimpleSocSC.cpp Or1ksimSC.h DataReporterSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartSC.o: UartSC.cpp UartSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

TermSC.o: TermSC.cpp TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# Clean up

.PHONY: clean
clean:
	$(RM) *.o
	$(RM) TestSC
	$(RM) SimpleSocSC
