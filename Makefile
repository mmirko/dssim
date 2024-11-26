#   Distributed System Simulator (dssim)
#   Copyright (C) 2012 2024 Mirko Mariotti
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

# Get the lua lib version from the installed lua library
LUA_LIB_VERSION = $(shell lua -e "print(string.match(_VERSION, '5%.[0-9]'))")

# Set LUA_VERSION to 5.3 if the headers are in include/lua5.3, 5.4 if they are in include/lua5.4, and to default if they are directly in include
LUA_VERSION = $(shell if [ -d /usr/include/lua5.3 ]; then echo 53; elif [ -d /usr/include/lua5.4 ]; then echo 54; else echo DEFAULT ; fi)


.PHONY: all
all: dssim dssim_gen 

.PHONY: clean
clean:
	@ rm -f *.o *.png *.avi dssim dssim_gen transformer.c
	@ for dir in proto_*; do $(MAKE) --no-print-directory -r -C $$dir clean; done

transformer.c : lua_embedder.py transformer.lua
	@ ./lua_embedder.py > transformer.c

dssim: dssim.c transformer.c messages.c list.h
	@ gcc -DLUA_VERSION_$(LUA_VERSION) -o dssim dssim.c -lm -lgvc -llua$(LUA_LIB_VERSION)  -lgd -lcgraph -lOpenCL

dssim_gen: dssim_gen.c
	@ gcc -o dssim_gen dssim_gen.c -lm -lgvc -lcgraph

.PHONY: test
test: dssim
	./dssim -p broadcast -i broadcast.init -g graphs_rep/tree4.dot -v 

.PHONY: regression
regression: dssim
	@ for dir in proto_*; do $(MAKE) --no-print-directory -r -C $$dir regression; done
