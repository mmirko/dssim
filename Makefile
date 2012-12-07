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
all: dssim graphtest

.phony: clean
clean:
	rm -f *.oct *.o *.log *.logp dssim *.png graphtest

dssim: dssim.c broadcast.cl
	gcc -I/usr/local/cuda/include -lOpenCL -o dssim dssim.c -lm -lgvc

graphtest: graphtest.c
	gcc -I/usr/local/cuda/include -lOpenCL -o graphtest graphtest.c -lgraph -lcdt

