#    Mxmms - An XMMS2 client with a large button.
#    Copyright (C) 2016 Yuuki Harano
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

all: mxmms

SRCS = main.c playlist.c playlists.c debug.c

mxmms: $(SRCS)
	cc -o mxmms $(SRCS) `pkg-config --cflags --libs libmatepanelapplet-4.0 xmms2-client xmms2-client-glib`

test: test.c mxmms_console.c
	cc -o test test.c mxmms_console.c `pkg-config --cflags --libs gtk+-3.0 xmms2-client`
