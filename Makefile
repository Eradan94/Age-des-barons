TARGET = age_des_barons
##########################################################"

CFLAGS = -g -O2 -Wall -Werror `pkg-config --cflags MLV`
LDLIBS = `pkg-config --libs MLV`

all: $(TARGET)

clean:
	-rm -rf $(TARGET)

#   This file is part of the MLV Library.
#
#   Copyright (C) 2010,2011,2012,2013 Adrien Boussicault, Marc Zipstein
#
#
#    This Library is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This Library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this Library.  If not, see <http://www.gnu.org/licenses/>.
