all: mxmms

mxmms: main.c mxmms_console.c
	cc -o mxmms main.c mxmms_console.c `pkg-config --cflags --libs libmatepanelapplet-4.0 xmms2-client xmms2-client-glib`
