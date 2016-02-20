all: mxmms

SRCS = main.c playlist.c playlists.c debug.c mxmms_console.c

mxmms: $(SRCS)
	cc -o mxmms $(SRCS) `pkg-config --cflags --libs libmatepanelapplet-4.0 xmms2-client xmms2-client-glib`

test: test.c mxmms_console.c
	cc -o test test.c mxmms_console.c `pkg-config --cflags --libs gtk+-3.0 xmms2-client`
