# ----------------------------------------------------------------------------

# Example Programs for "Building a Loosely Timed SoC Model with OSCI TLM 2.0"

# Copyright (C) 2008  Embecosm Limited <info@embecosm.com>

# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.

# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
# License for more details.

# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# ----------------------------------------------------------------------------

# Distribution construction

# $Id$

# Location

DISTNAME = embecosm-esp1-sysc-tlm2.0-examples-1.2


# ----------------------------------------------------------------------------
# Make the lot

.PHONY: dist
dist: distclean $(DISTNAME) progs_or32 models
	cp -r COPYING COPYING.LESSER INSTALL README \
		embecosm-or32-or1ksim-0.2.0-lib-patch.bz2 \
		linux.cfg simple.cfg sysc_models $(DISTNAME)
	$(RM) -r $(DISTNAME)/sysc_models/.svn
	$(RM) -r $(DISTNAME)/sysc_models/progs_or32/.svn
	tar -jcf $(DISTNAME).tar.bz2 ./$(DISTNAME)

$(DISTNAME):
	mkdir -p $(DISTNAME)

.PHONY: progs_or32
progs_or32:
	cd sysc_models/progs_or32 && $(MAKE) clean
	cd sysc_models/progs_or32 && $(MAKE) 
	cd sysc_models/progs_or32 && $(MAKE) doc
	cd sysc_models/progs_or32 && $(RM) *.o

.PHONY: models
models:
	cd sysc_models && $(MAKE) clean
	cd sysc_models && $(MAKE) doc


# ----------------------------------------------------------------------------
# Clean up

.PHONY: distclean
distclean:
	$(RM) -r build
	cd sysc_models && $(MAKE) clean
	cd sysc_models/progs_or32 && $(MAKE) clean
	$(RM) -r $(DISTNAME)
	$(RM)    $(DISTNAME).tar.bz2
