/* 
 * im-chooser.c
 * Copyright (C) 2006 Red Hat, Inc. All rights reserved.
 * 
 * Authors:
 *   Akira TAGOH  <tagoh@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "im-chooser.h"
#include "xinput.h"

#define ENCODE_MODE(mode, submode)		\
	(((submode) << 16) | ((mode) & 0xffff))
#define SET_MODE(val, mode)			\
	(val) = ((val) & 0xffff0000) | ((mode) & 0xffff)
#define SET_SUBMODE(val, submode)		\
	(val) = ((val) & 0xffff) | ((submode) << 16)
#define DECODE_MODE(val)			\
	((val) & 0xffff)
#define DECODE_SUBMODE(val)			\
	(((val) >> 16) & 0xffff)
		
#define IM_CHOOSER_MODE		"im-chooser-mode"
/* user config file */
#define IM_USER_XINPUT_CONF	".xinputrc"
/* global config file */
#define IM_GLOBAL_XINPUT_CONF	"xinputrc"
/* name that uses to be determined which IM is used for default. */
#define IM_DEFAULT_NAME		"xinputrc"
/* name that uses to be determined that IM is never used no matter what */
#define IM_NONE_NAME		"none"
/* name that uses to be determined that XIM is always used no matter what */
#define IM_XIM_NAME		"xim"
/* label that uses to indicate their own .xinputrc */
#define IM_USER_SPECIFIC_LABEL	"User Specific"
/* label that uses to indicate "unknown" xinput script */
#define IM_UNKNOWN_LABEL	"Unknown"


enum {
	CHANGED,
	LAST_SIGNAL
};

struct _IMChooserClass {
	GObjectClass parent_class;

	void (* changed) (IMChooser *im);
};

struct _IMChooser {
	GObject     parent_instance;
	GtkWidget  *widget;
	GtkWidget  *option;
	GSList     *widget_groups;
	GHashTable *lang_table;
	GHashTable *im_table;
	gboolean    modified;
	gboolean    is_backedup;
	gulong      changed_signal_id;
	gint32      mode;
};

static void _im_chooser_button_clicked_cb(GtkButton     *button,
					  gpointer       data);
static void _im_chooser_option_changed_cb(GtkOptionMenu *menu,
					  gpointer       data);

static gint32 _im_chooser_detect_current_mode   (void);
static void   _im_chooser_set_tooltips          (GtkWidget   *widget,
						 const gchar *tip);
static void   _im_chooser_update_custom_im_menu (IMChooser   *im);
static void   _im_chooser_set_menu_from_im_table(gpointer     key,
						 gpointer     val,
						 gpointer     data);


static GObjectClass *parent_class = NULL;
static guint         signals[LAST_SIGNAL] = { 0 };


/*
 * signal callback functions
 */
static void
im_chooser_real_finalize(GObject *object)
{
	IMChooser *im;

	g_return_if_fail (IM_IS_CHOOSER (object));

	im = IM_CHOOSER (object);

	if (im->im_table)
		g_hash_table_destroy(im->im_table);
	if (im->lang_table)
		g_hash_table_destroy(im->lang_table);
}

static void
_im_chooser_button_clicked_cb(GtkButton *button,
			      gpointer   data)
{
	IMChooser *im;
	IMChooserMode mode;
	const gchar *name;
	GtkWidget *menu, *menuitem;

	g_return_if_fail (IM_IS_CHOOSER (data));

	im = IM_CHOOSER (data);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button))) {
		mode = GPOINTER_TO_INT (g_object_get_data(G_OBJECT (button), IM_CHOOSER_MODE));
		if (mode != DECODE_MODE (im->mode)) {
			menu = gtk_option_menu_get_menu(GTK_OPTION_MENU (im->option));
			menuitem = gtk_menu_get_active(GTK_MENU (menu));
			if (menuitem != NULL)
				name = g_object_get_data(G_OBJECT (menuitem), "user-data");
			else
				name = "";
			if (!im_chooser_update_xinputrc(im, mode, IM_SUBMODE_SYMLINK, name)) {
				/* FIXME */
			} else {
				if (DECODE_MODE (im->mode) == IM_MODE_CUSTOM) {
					if (DECODE_SUBMODE (im->mode) != IM_SUBMODE_SYMLINK) {
						SET_SUBMODE (im->mode, IM_SUBMODE_SYMLINK);
						_im_chooser_update_custom_im_menu(im);
					}
					gtk_widget_set_sensitive(im->option, FALSE);
				}
			}
			SET_MODE (im->mode, mode);
			im->modified = TRUE;
			g_signal_emit(im, signals[CHANGED], 0);
		}
	}
}

static void
_im_chooser_option_changed_cb(GtkOptionMenu *optionmenu,
			      gpointer       data)
{
	IMChooser *im;
	GtkWidget *menu, *menuitem;
	const gchar *name;

	g_return_if_fail (IM_IS_CHOOSER (data));

	im = IM_CHOOSER (data);
	menu = gtk_option_menu_get_menu(optionmenu);
	menuitem = gtk_menu_get_active(GTK_MENU (menu));
	name = g_object_get_data(G_OBJECT (menuitem), "user-data");
	if (strcmp(name, IM_USER_SPECIFIC_LABEL) == 0 ||
	    strcmp(name, IM_UNKNOWN_LABEL) == 0) {
		/* it may be not changed. usually it's unlikely to happen this from other mode */
		return;
	}
	if (!im_chooser_update_xinputrc(im, IM_MODE_CUSTOM, IM_SUBMODE_SYMLINK, name)) {
		/* FIXME */
	} else {
		if (DECODE_SUBMODE (im->mode) != IM_SUBMODE_SYMLINK) {
			SET_SUBMODE (im->mode, IM_SUBMODE_SYMLINK);
			_im_chooser_update_custom_im_menu(im);
		}
		im->modified = TRUE;
		g_signal_emit(im, signals[CHANGED], 0);
	}
}

/*
 * Private Functions
 */
static void
im_chooser_class_init(IMChooserClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);

	/* initialize GObject */
	gobject_class->finalize = im_chooser_real_finalize;

	/* signals */
	signals[CHANGED] = g_signal_new("changed",
					G_OBJECT_CLASS_TYPE (klass),
					G_SIGNAL_RUN_FIRST,
					G_STRUCT_OFFSET (IMChooserClass, changed),
					NULL, NULL,
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE, 0);
}

static void
im_chooser_instance_init(IMChooser *im)
{
	GSList *l, *im_list;
	XInputData *xinput;

	im->modified = FALSE;
	im->is_backedup = FALSE;

	/* read system-wide info */
	im->im_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, xinput_data_free);
	im->lang_table = xinput_get_lang_table(NULL);
	/* get all the info of xinput script */
	im_list = xinput_get_im_list(NULL);
	for (l = im_list; l != NULL; l = g_slist_next(l)) {
		xinput = xinput_data_new(l->data);
		if (xinput != NULL) {
			gchar *name = g_path_get_basename(l->data);
			size_t suffixlen = strlen(XINPUT_SUFFIX), len;

			if (name != NULL &&
			    GPOINTER_TO_UINT (xinput_data_get_value(xinput, XINPUT_VALUE_IGNORE_ME)) != TRUE) {
				len = strlen(name);
				if (len > suffixlen) {
					name[len - suffixlen] = 0;
					g_hash_table_replace(im->im_table, name, xinput);
				}
			}
		}
	}

	/* detect current mode and cache it */
	im->mode = _im_chooser_detect_current_mode();
}

static gboolean
_im_chooser_detect_real_current_mode(gint32      *ret,
				     const gchar *xinputrc)
{
	gboolean retval = FALSE;
	IMChooserMode mode = IM_MODE_CUSTOM;
	IMChooserSubMode submode = IM_SUBMODE_UNKNOWN;
	gchar *origname, *name;
	struct stat st;

	if (lstat(xinputrc, &st) == 0) {
		if (S_ISLNK (st.st_mode)) {
			origname = g_file_read_link(xinputrc, NULL);
			if (origname && stat(origname, &st) == 0) {
				name = g_path_get_basename(origname);
				if (strcmp(name, IM_GLOBAL_XINPUT_CONF) == 0) {
					/* need to check the global conf. otherwise it may be invalid */
					mode = IM_MODE_SYSTEM;
					retval = TRUE;
				} else if (strcmp(name, IM_DEFAULT_NAME) == 0) {
					/* FIXME: need to check the value of alternatives.
					 *        if it points to xim or none, it shouldn't be
					 *        IM_MODE_SYSTEM.
					 */
					mode = IM_MODE_SYSTEM;
					retval = TRUE;
				} else if (strcmp(name, IM_NONE_NAME XINPUT_SUFFIX) == 0) {
					mode = IM_MODE_NEVER;
					retval = TRUE;
				} else if (strcmp(name, IM_XIM_NAME XINPUT_SUFFIX) == 0) {
					mode = IM_MODE_LEGACY;
					retval = TRUE;
				} else {
					submode = IM_SUBMODE_SYMLINK;
					retval = TRUE;
				}
				g_free(name);
				g_free(origname);
			}
		} else {
			submode = IM_SUBMODE_USER;
			retval = TRUE;
		}
		*ret = ENCODE_MODE (mode, submode);
	}

	return retval;
}

static gint32
_im_chooser_detect_current_mode(void)
{
	gchar *filename;
	const gchar *home;
	gint32 mode = ENCODE_MODE (IM_MODE_CUSTOM, IM_SUBMODE_UNKNOWN);

	home = g_get_home_dir();
	if (home == NULL) {
		g_error("Failed to get a place of home directory.");
		return mode;
	}
	filename = g_build_filename(home, IM_USER_XINPUT_CONF, NULL);
	if (filename != NULL) {
		struct stat st;

		if (lstat(filename, &st) == -1) {
			/* no .xinputrc file. follow the system-wide configuration */
			mode = ENCODE_MODE (IM_MODE_SYSTEM, IM_SUBMODE_UNKNOWN);
		} else {
			if (!_im_chooser_detect_real_current_mode(&mode, filename)) {
				/* try to detect from the global xinputrc */
				g_free(filename);
				filename = g_build_filename(XINIT_PATH, IM_GLOBAL_XINPUT_CONF, NULL);
				if (filename != NULL &&
				    !_im_chooser_detect_real_current_mode(&mode, filename)) {
					/* probably unknown mode */
					g_warning("Failed to detect the current mode from .xinputrc/xinputrc.");
				}
			}
		}
	}
	if (filename)
		g_free(filename);

	return mode;
}

static void
_im_chooser_set_tooltips(GtkWidget   *widget,
			 const gchar *tip)
{
	GtkTooltips *tooltips = gtk_tooltips_new();

	gtk_tooltips_set_tip(tooltips, widget, tip, NULL);
}

static void
_im_chooser_update_custom_im_menu(IMChooser *im)
{
	GtkWidget *menu;

	g_return_if_fail (im != NULL);

	menu = gtk_menu_new();

	if (im->changed_signal_id > 0)
		g_signal_handler_disconnect(im->option, im->changed_signal_id);

	g_hash_table_foreach(im->im_table, _im_chooser_set_menu_from_im_table, menu);
	if (DECODE_MODE (im->mode) == IM_MODE_CUSTOM) {
		if (DECODE_SUBMODE (im->mode) == IM_SUBMODE_USER) {
			GtkWidget *menuitem;
			gchar *label = N_(IM_USER_SPECIFIC_LABEL);

			menuitem = gtk_menu_item_new_with_label(_(label));
			g_object_set_data(G_OBJECT (menuitem), "user-data", label);
			gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
			/* ugly hack */
			gtk_menu_set_active(GTK_MENU (menu), g_list_length(GTK_MENU_SHELL (menu)->children) - 1);
			gtk_widget_show(menuitem);
		} else if (DECODE_SUBMODE (im->mode) == IM_SUBMODE_UNKNOWN) {
			GtkWidget *menuitem;
			gchar *label = N_(IM_UNKNOWN_LABEL);

			menuitem = gtk_menu_item_new_with_label(_(label));
			g_object_set_data(G_OBJECT (menuitem), "user-data", label);
			gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
			/* ugly hack */
			gtk_menu_set_active(GTK_MENU (menu), g_list_length(GTK_MENU_SHELL (menu)->children) - 1);
			gtk_widget_show(menuitem);
		}
	}
	gtk_option_menu_set_menu(GTK_OPTION_MENU (im->option), menu);

	im->changed_signal_id = g_signal_connect(im->option, "changed",
						 G_CALLBACK (_im_chooser_option_changed_cb), im);
}

static void
_im_chooser_set_menu_from_im_table(gpointer key,
				   gpointer val,
				   gpointer data)
{
	GtkWidget *menu = data;
	GtkWidget *menuitem;
	gchar *name, *origname, *filename;
	const gchar *home;

	if (strcmp(key, IM_DEFAULT_NAME) != 0 &&
	    strcmp(key, IM_NONE_NAME) != 0 &&
	    strcmp(key, IM_XIM_NAME) != 0) {
		menuitem = gtk_menu_item_new_with_label(key);
		g_object_set_data(G_OBJECT (menuitem), "user-data", key);
		gtk_menu_shell_append(GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show(menuitem);
	}
	home = g_get_home_dir();
	if (home == NULL) {
		g_error("Failed to get a place of home directory.");
		return;
	}
	filename = g_build_filename(home, IM_USER_XINPUT_CONF, NULL);
	if (filename != NULL &&
	    (origname = g_file_read_link(filename, NULL)) != NULL) {
		gchar *confname;

		name = g_path_get_basename(origname);
		/* it may be easier that just comparing with + suffix instead
		 * of getting rid of suffix from name.
		 */
		confname = g_strdup_printf("%s%s", (gchar *)key, XINPUT_SUFFIX);
		if (strcmp(name, key) == 0 ||
		    strcmp(name, confname) == 0) {
			/* ugly hack */
			gtk_menu_set_active(GTK_MENU (menu),
					    g_list_length(GTK_MENU_SHELL (menu)->children) - 1);
		}
		g_free(confname);
		g_free(name);
		g_free(origname);
	}
	if (filename)
		g_free(filename);
}

/*
 * Public Functions
 */
GType
im_chooser_get_type(void)
{
	static GType im_type = 0;

	if (!im_type) {
		static const GTypeInfo im_info = {
			.class_size     = sizeof (IMChooserClass),
			.base_init      = NULL,
			.base_finalize  = NULL,
			.class_init     = (GClassInitFunc)im_chooser_class_init,
			.class_finalize = NULL,
			.class_data     = NULL,
			.instance_size  = sizeof (IMChooser),
			.n_preallocs    = 0,
			.instance_init  = (GInstanceInitFunc)im_chooser_instance_init,
			.value_table    = NULL,
		};

		im_type = g_type_register_static(G_TYPE_OBJECT, "IMChooser",
						 &im_info, 0);
	}

	return im_type;
}

IMChooser *
im_chooser_new(void)
{
	return IM_CHOOSER (g_object_new(IM_TYPE_CHOOSER, NULL));
}

GtkWidget *
im_chooser_get_widget(IMChooser *im)
{
	GtkWidget *vbox, *label, *radio, *hbox, *hbox2, *label2, *vbox2, *align, *align2, *label3;
	gint i;
	GSList *l;
	XInputData *xinput;
	const gchar *label_name = NULL;
	gchar *p, *global;
	gboolean no_xim = FALSE, no_custom = FALSE;

	g_return_val_if_fail (IM_IS_CHOOSER (im), NULL);

	if (im->widget == NULL) {
		/* setup widgets */
		vbox = gtk_vbox_new(FALSE, 0);
		vbox2 = gtk_vbox_new(FALSE, 0);
		hbox = gtk_hbox_new(FALSE, 0);
		hbox2 = gtk_hbox_new(FALSE, 0);
		align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
		align2 = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);

		label3 = gtk_label_new(_("<small><i>Note: this change will not take effect until you next log in.</i></small>"));
		gtk_label_set_use_markup(GTK_LABEL (label3), TRUE);
		gtk_label_set_line_wrap(GTK_LABEL (label3), TRUE);

		label = gtk_label_new(_("<b>Input Method</b>"));
		gtk_label_set_use_markup(GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment(GTK_MISC (label), 0, 0);

		global = g_build_filename(XINIT_PATH, IM_GLOBAL_XINPUT_CONF, NULL);
		xinput = xinput_data_new(global);
		if (xinput != NULL) {
			label_name = xinput_data_get_value(xinput, XINPUT_VALUE_XIM);
		}
		if (label_name == NULL) {
			label_name = _("No valid IM installed");
		}
		p = g_strdup_printf("[%s]", label_name);
		label2 = gtk_label_new(p);
		g_free(p);
		if (xinput)
			xinput_data_free(xinput);

		/* check if there are any valid XIM servers installed */
		if (g_hash_table_size(im->lang_table) == 0)
			no_xim = TRUE;

		/* check if there are any valid custom IM installed */
		if (g_hash_table_size(im->im_table) == 0)
			no_custom = TRUE;

		/* follow the system default */
		radio = gtk_radio_button_new_with_mnemonic(NULL,
							   _("_Follow the system-wide configuration"));
		im->widget_groups = g_slist_append(im->widget_groups, radio);
		_im_chooser_set_tooltips(radio, _("Follow the system-wide input method configuration to determine which input method should be used."));
		/* never use input methods */
		radio = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON (radio),
								       _("_Never use input methods"));
		im->widget_groups = g_slist_append(im->widget_groups, radio);
		_im_chooser_set_tooltips(radio, _("Entirely disable input methods regardless which locale the desktop is running on."));
		/* use legacy input methods */
		radio = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON (radio),
								       _("_Use legacy input methods"));
		im->widget_groups = g_slist_append(im->widget_groups, radio);
		_im_chooser_set_tooltips(radio, _("Use X Input Method anyway. In fact, which input method will be chosen for the desktop session, depends on the desktop language."));
		if (no_xim) {
			gtk_widget_set_sensitive(radio, FALSE);
		}
		/* custom input methods */
		radio = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON (radio),
								       _("Use _custom input method"));
		im->widget_groups = g_slist_append(im->widget_groups, radio);
		_im_chooser_set_tooltips(radio, _("Use input method that you prefer regardless what input method the system recommends."));
		if (no_custom) {
			gtk_widget_set_sensitive(radio, FALSE);
		}

		/* for custom IM */
		im->option = gtk_option_menu_new();
		_im_chooser_update_custom_im_menu(im);

		/* packs widgets */
		gtk_container_set_border_width(GTK_CONTAINER (vbox), 10);
		gtk_alignment_set_padding(GTK_ALIGNMENT (align), 7, 0, 12, 0);
		gtk_box_pack_start(GTK_BOX (vbox), label, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX (vbox), align, TRUE, TRUE, 0);
		gtk_container_add(GTK_CONTAINER (align), vbox2);
		for (i = IM_MODE_SYSTEM, l = im->widget_groups; l != NULL; l = g_slist_next(l), i++) {
			g_object_set_data(G_OBJECT (l->data), IM_CHOOSER_MODE, GINT_TO_POINTER (i));
			if (i == IM_MODE_SYSTEM) {
				gtk_box_pack_start(GTK_BOX (hbox2), l->data, FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX (hbox2), label2, FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
			} else if (i == IM_MODE_CUSTOM) {
				gtk_box_pack_start(GTK_BOX (hbox), l->data, FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX (hbox), im->option, TRUE, TRUE, 0);
				gtk_box_pack_start(GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
				gtk_widget_set_sensitive(im->option, FALSE);
			} else {
				gtk_box_pack_start(GTK_BOX (vbox2), l->data, TRUE, TRUE, 0);
			}
			/* signal connections */
			g_signal_connect(l->data, "clicked",
					 G_CALLBACK (_im_chooser_button_clicked_cb), im);

			if (i == DECODE_MODE (im->mode))
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (l->data), TRUE);
		}
		gtk_alignment_set_padding(GTK_ALIGNMENT (align2), 10, 0, 0, 0);
		gtk_container_add(GTK_CONTAINER (align2), label3);
		gtk_box_pack_start(GTK_BOX (vbox), align2, FALSE, FALSE, 0);
		if (DECODE_MODE (im->mode) == IM_MODE_CUSTOM && !no_custom)
			gtk_widget_set_sensitive(im->option, TRUE);

		im->widget = vbox;
	}

	return im->widget;
}

void
im_chooser_get_current_mode(IMChooser        *im,
			    IMChooserMode    *mode,
			    IMChooserSubMode *submode)
{
	g_return_if_fail (IM_IS_CHOOSER (im));
	g_return_if_fail (mode != NULL);
	g_return_if_fail (submode != NULL);

	*mode = DECODE_MODE (im->mode);
	*submode = DECODE_SUBMODE (im->mode);
}

gboolean
im_chooser_update_xinputrc(IMChooser        *im,
			   IMChooserMode     mode,
			   IMChooserSubMode  submode,
			   const gchar      *xinputname)
{
	gchar *srcfile = NULL, *dstfile = NULL, *backupfile = NULL, *confname;
	const gchar *home;
	gboolean retval = FALSE;
	struct stat st;

	g_return_val_if_fail (IM_IS_CHOOSER (im), FALSE);

	home = g_get_home_dir();
	if (home == NULL) {
		g_error("Failed to get a place of home directory.");
		return FALSE;
	}
	dstfile = g_build_filename(home, IM_USER_XINPUT_CONF, NULL);
	if (lstat(dstfile, &st) == 0) {
		if (!im->is_backedup) {
			backupfile = g_build_filename(home, IM_USER_XINPUT_CONF ".bak", NULL);
			if (rename(dstfile, backupfile) == -1) {
				g_warning("Failed to create a backup file.");
				g_free(backupfile);
				return FALSE;
			}
			im->is_backedup = TRUE;
		} else {
			if (unlink(dstfile) == -1) {
				g_warning("Failed to remove %s", dstfile);
				g_free(dstfile);
				return FALSE;
			}
		}
	}
	switch (mode) {
	    case IM_MODE_SYSTEM:
		    /* no need to do anything. no .xinputrc means that
		     * it follows the system-wide configuration.
		     */
		    retval = TRUE;
		    break;
	    case IM_MODE_NEVER:
		    srcfile = g_build_filename(XINPUT_PATH, IM_NONE_NAME XINPUT_SUFFIX, NULL);
		    if (symlink(srcfile, dstfile) == -1) {
			    g_warning("Failed to create a symlink %s from %s", dstfile, srcfile);
		    }
		    retval = TRUE;
		    break;
	    case IM_MODE_LEGACY:
		    srcfile = g_build_filename(XINPUT_PATH, IM_XIM_NAME XINPUT_SUFFIX, NULL);
		    if (symlink(srcfile, dstfile) == -1) {
			    g_warning("Failed to create a symlink %s from %s", dstfile, srcfile);
		    }
		    retval = TRUE;
		    break;
	    case IM_MODE_CUSTOM:
		    switch (submode) {
			case IM_SUBMODE_USER:
			case IM_SUBMODE_UNKNOWN:
				g_warning("User specific/Unknown submode isn't allowed to set directly.");
				break;
			case IM_SUBMODE_SYMLINK:
				if (xinputname == NULL) {
					g_warning("no xinputname given.");
					break;
				}
				confname = g_strdup_printf("%s%s", xinputname, XINPUT_SUFFIX);
				srcfile = g_build_filename(XINPUT_PATH, confname, NULL);
				g_free(confname);
				if (symlink(srcfile, dstfile) == -1) {
					g_warning("Failed to create a symlink %s from %s", dstfile, srcfile);
				}
				if (im->option)
					gtk_widget_set_sensitive(im->option, TRUE);
				retval = TRUE;
			default:
				break;
		    }
		    break;
	    default:
		    break;
	}
	if (srcfile)
		g_free(srcfile);
	if (dstfile)
		g_free(dstfile);

	return retval;
}

gboolean
im_chooser_validate_mode(IMChooser        *im,
			 IMChooserMode     mode,
			 IMChooserSubMode  submode)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (IM_IS_CHOOSER (im), FALSE);

	switch (mode) {
	    case IM_MODE_SYSTEM:
		    if (g_hash_table_lookup(im->im_table, IM_DEFAULT_NAME) != NULL)
			    retval = TRUE;
		    break;
	    case IM_MODE_LEGACY:
		    if (g_hash_table_size(im->lang_table) > 0)
			    retval = TRUE;
		    break;
	    case IM_MODE_NEVER:
		    retval = TRUE;
		    break;
	    case IM_MODE_CUSTOM:
		    switch (submode) {
			case IM_SUBMODE_USER:
			case IM_SUBMODE_UNKNOWN:
			case IM_SUBMODE_SYMLINK:
				retval = TRUE;
				break;
			default:
				break;
		    }
		    break;
	    default:
		    break;
	}

	return retval;
}

gboolean
im_chooser_is_modified(IMChooser *im)
{
	g_return_val_if_fail (IM_IS_CHOOSER (im), FALSE);

	return im->modified;
}
