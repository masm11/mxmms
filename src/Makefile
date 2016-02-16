all: mxmms

SRCS = main.c playlist.c debug.c

mxmms: $(SRCS)
	cc -o mxmms $(SRCS) `pkg-config --cflags --libs libmatepanelapplet-4.0 xmms2-client xmms2-client-glib`
