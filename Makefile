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

LUA_VERSIONS=54
LUA_VERSION=5.4

.PHONY: all
all: dssim dssim_gen 

.PHONY: clean
clean:
	@ rm -f *.o *.png *.avi dssim dssim_gen transformer.c
	@ for dir in proto_*; do $(MAKE) --no-print-directory -r -C $$dir clean; done

transformer.c : lua_embedder.py transformer.lua
	@ ./lua_embedder.py > transformer.c

dssim: dssim.c transformer.c messages.c list.h
	@ gcc -DLUA_VERSION_$(LUA_VERSIONS) -o dssim dssim.c -lm -lgvc -llua$(LUA_VERSION)  -lgd -lcgraph -lOpenCL

dssim_gen: dssim_gen.c
	@ gcc -o dssim_gen dssim_gen.c -lm -lgvc -lcgraph

.PHONY: test
test: dssim
	./dssim -p broadcast -i broadcast.init -g graphs_rep/tree4.dot -v 

.PHONY: regression
regression: dssim
	@ for dir in proto_*; do $(MAKE) --no-print-directory -r -C $$dir regression; done
