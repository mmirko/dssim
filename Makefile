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


.PHONY: all
all: dssim dssim_gen flooding_early_tree4 flooding_early_lattice flooding_mid_lattice flooding_lattice flooding_hypercube

.PHONY: clean
clean:
	@ rm -f *.o *.png *.avi dssim dssim_gen transformer.c outfile*
	@ $(MAKE) --no-print-directory -r -C flooding_early_lattice clean
	@ $(MAKE) --no-print-directory -r -C flooding_early_tree4 clean
	@ $(MAKE) --no-print-directory -r -C flooding_mid_lattice clean
	@ $(MAKE) --no-print-directory -r -C flooding_lattice clean
	@ $(MAKE) --no-print-directory -r -C flooding_hypercube clean

transformer.c : lua_embedder.py transformer.lua
	@ ./lua_embedder.py > transformer.c

dssim: dssim.c transformer.c
	@ gcc -lOpenCL -o dssim dssim.c -lm -lgvc -llua -lgd

dssim_gen: dssim_gen.c
	@ gcc -o dssim_gen dssim_gen.c -lm -lgvc

.PHONY: flooding_early_lattice
flooding_early_lattice:
	@ $(MAKE) --no-print-directory -r -C flooding_early_lattice

.PHONY: flooding_early_tree4
flooding_early_tree4:
	@ $(MAKE) --no-print-directory -r -C flooding_early_tree4

.PHONY: flooding_mid_lattice
flooding_mid_lattice:
	@ $(MAKE) --no-print-directory -r -C flooding_mid_lattice

.PHONY: flooding_lattice
flooding_lattice:
	@ $(MAKE) --no-print-directory -r -C flooding_lattice

.PHONY: flooding_hypercube
flooding_hypercube:
	@ $(MAKE) --no-print-directory -r -C flooding_hypercube

.PHONY: regression
regression:
	@ $(MAKE) --no-print-directory -r -C flooding_lattice regression
	@ $(MAKE) --no-print-directory -r -C flooding_hypercube regression
