#   Distributed System Simulator (dssim)
#   Copyright (C) 2012 Mirko Mariotti
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


.phony: all
all: dssim dssim_gen flooding_early_tree4 flooding_early_lattice flooding_mid_lattice flooding_lattice

.phony: clean
clean:
	rm -f *.o *.png *.avi dssim dssim_gen transformer.c outfile*
	$(MAKE) -r -C flooding_early_lattice clean
	$(MAKE) -r -C flooding_early_tree4 clean
	$(MAKE) -r -C flooding_mid_lattice clean
	$(MAKE) -r -C flooding_lattice clean

transformer.c : lua_embedder.py transformer.lua
	./lua_embedder.py > transformer.c

dssim: dssim.c transformer.c
	gcc -lOpenCL -o dssim dssim.c -lm -lgvc -llua

dssim_gen: dssim_gen.c
	gcc -o dssim_gen dssim_gen.c -lm -lgvc

.phony: flooding_early_lattice
flooding_early_lattice:
	$(MAKE) -r -C flooding_early_lattice all

.phony: flooding_early_tree4
flooding_early_tree4:
	$(MAKE) -r -C flooding_early_tree4 all

.phony: flooding_mid_lattice
flooding_mid_lattice:
	$(MAKE) -r -C flooding_mid_lattice all

.phony: flooding_lattice
flooding_lattice:
	$(MAKE) -r -C flooding_lattice all

