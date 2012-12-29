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
all: dssim

.phony: clean
clean:
	@ rm -f *.o *.png *.avi *.tmp dssim dssim_gen dssim_video transformer.c outfile*

.phony: dssim
dssim:
	@ ln -sf ../dssim dssim
	@ ln -sf ../dssim_gen dssim_gen
	@ ln -sf ../dssim_video dssim_video

.phony: video
video: dssim
	@ ./dssim_video -i flooding.init -p flooding -g lattice10.dot -T neato

.phony: regression
regression: dssim
	@ ./dssim -i flooding.init -p flooding -g lattice10.dot > regression.tmp
	@ (cmp regression.tmp regression.dat > /dev/null 2>&1  && echo "OK   -> flooding-lattice" ) || echo "FAIL -> flooding-lattice"

.phony: regression-update
regression-update: dssim
	@ ./dssim -i flooding.init -p flooding -g lattice10.dot > regression.dat
