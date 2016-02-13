all: mxmms

mxmms: main.c
	cc -o mxmms main.c `pkg-config --cflags --libs libmatepanelapplet-4.0 xmms2-client xmms2-client-glib`
