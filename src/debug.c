#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>
#include "common.h"


static void print_indent(int indent);
static void print_dict_entry(const gchar *key, xmmsv_t *value, void *user_data);

static void print_indent(int indent)
{
    for (gint i = 0; i < indent; i++)
	putchar(' ');
}

static void print_dict_entry(const gchar *key, xmmsv_t *value, void *user_data)
{
    gint indent = GPOINTER_TO_INT(user_data);
    print_indent(indent);
    printf("key: %s\n", key);
    print_indent(indent);
    printf("val: \n");
    print_xmmsv(value, indent + 2);
}

void print_xmmsv(xmmsv_t *val, int indent)
{
    const gchar *str;
    gint64 n64;
    switch (xmmsv_get_type(val)) {
    case XMMSV_TYPE_NONE:
	print_indent(indent);
	printf("none: \n");
	break;
    case XMMSV_TYPE_ERROR:
	if (xmmsv_get_error(val, &str)) {
	    print_indent(indent);
	    printf("error: %s\n", str);
	} else {
	    print_indent(indent);
	    printf("error: unknown\n");
	}
	break;
    case XMMSV_TYPE_INT64:
	if (xmmsv_get_int64(val, &n64)) {
	    print_indent(indent);
	    printf("int64: %ld\n", n64);
	} else {
	    print_indent(indent);
	    printf("int64: unknown.\n");
	}
	break;
    case XMMSV_TYPE_STRING:
	if (xmmsv_get_string(val, &str)) {
	    print_indent(indent);
	    printf("string: %s\n", str);
	} else {
	    print_indent(indent);
	    printf("string: unknown\n");
	}
	break;
    case XMMSV_TYPE_COLL:
	print_indent(indent);
	printf("coll: \n");
	break;
	
    case XMMSV_TYPE_BIN:
	print_indent(indent);
	printf("bin: \n");
	break;
	
    case XMMSV_TYPE_LIST:
	print_indent(indent);
	printf("list: \n");
	{
	    xmmsv_list_iter_t *it;
	    if (xmmsv_get_list_iter(val, &it)) {
		do {
		    xmmsv_t *v;
		    xmmsv_list_iter_entry(it, &v);
		    print_xmmsv(v, indent + 2);
		} while (xmmsv_list_iter_next(it), xmmsv_list_iter_valid(it));
	    }
	}
	break;
	
    case XMMSV_TYPE_DICT:
	print_indent(indent);
	printf("dict: \n");
	xmmsv_dict_foreach(val, print_dict_entry, GINT_TO_POINTER(indent + 2));
	break;
	
    case XMMSV_TYPE_BITBUFFER:
	print_indent(indent);
	printf("bitbuf: \n");
	break;
	
    case XMMSV_TYPE_FLOAT:
	print_indent(indent);
	printf("float: \n");
	break;
    }
}
