# ----------------------------------------------------------------------------

# Example Programs for "Building a Loosely Timed SoC Model with OSCI TLM 2.0"

# Copyright (C) 2008  Embecosm Limited

# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
# License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ----------------------------------------------------------------------------

# Linux makefile for OSCI SystemC wrapper project main wrapper

# $Id$

# Tools

CXX       = g++
CXXFLAGS += -DSC_INCLUDE_DYNAMIC_PROCESSES

# Where stuff is

OR1KSIM    = /opt/or1ksim
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
all: TestSC SimpleSocSC SyncSocSC DecoupSocSC IntrSocSC


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

SimpleSocSC: SimpleSocSC.o Or1ksimExtSC.o Or1ksimSC.o UartSC.o TermSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

SimpleSocSC.o: SimpleSocSC.cpp Or1ksimExtSC.h Or1ksimSC.h UartSC.h TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimExtSC.o: Or1ksimExtSC.cpp Or1ksimExtSC.h Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartSC.o: UartSC.cpp UartSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

TermSC.o: TermSC.cpp TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# Synchronous SoC with the synchronous ISS

SyncSocSC: SyncSocSC.o Or1ksimSyncSC.o Or1ksimExtSC.o Or1ksimSC.o \
	   UartSyncSC.o UartSC.o TermSyncSC.o TermSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

SyncSocSC.o: SyncSocSC.cpp Or1ksimSyncSC.h Or1ksimExtSC.h Or1ksimSC.h \
	     UartSyncSC.h UartSC.h TermSyncSC.h TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimSyncSC.o: Or1ksimSyncSC.cpp Or1ksimSyncSC.h Or1ksimExtSC.h Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartSyncSC.o: UartSyncSC.cpp UartSyncSC.h UartSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

TermSyncSC.o: TermSyncSC.cpp TermSyncSC.h TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# Temporally decoupled SoC

DecoupSocSC: DecoupSocSC.o Or1ksimDecoupSC.o Or1ksimSyncSC.o Or1ksimExtSC.o \
	     Or1ksimSC.o UartDecoupSC.o UartSyncSC.o UartSC.o TermSyncSC.o \
	     TermSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

DecoupSocSC.o: DecoupSocSC.cpp Or1ksimDecoupSC.h Or1ksimSyncSC.h \
	       Or1ksimExtSC.h Or1ksimSC.h UartDecoupSC.h UartSyncSC.h \
	       UartSC.h TermSyncSC.h TermSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimDecoupSC.o: Or1ksimDecoupSC.cpp Or1ksimDecoupSC.h Or1ksimSyncSC.h \
	           Or1ksimExtSC.h Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartDecoupSC.o: UartDecoupSC.cpp UartDecoupSC.h UartSyncSC.h UartSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<


# ----------------------------------------------------------------------------
# SoC which handles interrupts

IntrSocSC: IntrSocSC.o Or1ksimIntrSC.o Or1ksimDecoupSC.o Or1ksimSyncSC.o \
	   Or1ksimExtSC.o Or1ksimSC.o UartIntrSC.o UartDecoupSC.o \
	   UartSyncSC.o UartSC.o TermSyncSC.o TermSC.o
	$(CXX) $(CXXFLAGS) $^ -Wl,--rpath,$(OR1KSIMLIB) \
		$(LIBDIRS) $(LIBS) -o $@

IntrSocSC.o: IntrSocSC.cpp Or1ksimIntrSC.h Or1ksimDecoupSC.h Or1ksimSyncSC.h \
	       Or1ksimExtSC.h Or1ksimSC.h UartDecoupSC.h UartSyncSC.h \
	       UartSC.h TermSyncSC.h TermSC.h 
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

Or1ksimIntrSC.o: Or1ksimIntrSC.cpp Or1ksimIntrSC.h Or1ksimDecoupSC.h \
		 Or1ksimSyncSC.h Or1ksimExtSC.h Or1ksimSC.h
	$(CXX) $(CXXFLAGS) $(INCDIRS) -c $<

UartIntrSC.o: UartIntrSC.cpp UartIntrSC.h UartDecoupSC.h UartSyncSC.h UartSC.h
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
	$(RM)    DecoupSocSC
	$(RM)    IntrSocSC
