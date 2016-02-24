/*    Mxmms - An XMMS2 client with a large button.
 *    Copyright (C) 2016 Yuuki Harano
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
