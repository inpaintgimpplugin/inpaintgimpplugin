/* interface.c  --- inpaintBCT
 * Copyright (C) 2013 Thomas März (maerz@maths.ox.ac.uk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <string.h>

#include "main.h"
#include "interface.h"

#include "plugin-intl.h"


/*  Constants  */

#define SCALE_WIDTH 125
#define BOX_SPACING 6
#define SCALE_MAX 20
#define CONV_MAX  100

#define EPS_DIGITS 1
#define KAPPA_DIGITS 2
#define SMOOTH_DIGITS 3

#define RED_COEFF_DEFAULT          30
#define GREEN_COEFF_DEFAULT    59
#define BLUE_COEFF_DEFAULT    11

#define PREVIEW_SIZE    300




/*  Local function prototypes  */

static gboolean   dialog_image_constraint_func (gint32    image_id,
                                                gpointer  data);


/*  Local variables  */

static PlugInUIVals *ui_state = NULL;

typedef struct {
	GtkObject *epsilon_scale;
	GtkObject *kappa_scale;
	GtkObject *sigma_scale;
	GtkObject *rho_scale;
	GtkObject *threshold_scale;
	gchar *image_name;
	gint32 imageID;
	gboolean preview;
	gint     previewWidth;
	gint     previewHeight;
	GtkWidget *preview_widget;
	GimpDrawable *image_drawable;
	GimpDrawable *mask_drawable;
	GtkWidget* mask_combo_widget;
	GtkWidget* stop_path_combo_widget;
	GtkWidget* mask_type_widget;
	enum MaskType mask_type;
	gint  				 selectionX0;
	gint                 selectionY0;
	gint                 selectionX1;
	gint                 selectionY1;
	gint                 selectionWidth;
	gint                 selectionHeight;
	guchar *previewImage;
	guchar *previewMask;
	guchar *previewStopPath;
	guchar *previewResult;
	//enum MaskType mask_type;
	//gboolean selection;
	gboolean use_stop_path;

} InterfaceVals;
static InterfaceVals interface_vals;

static void util_fillReducedBuffer (guchar       *dest,
                                    gint          destWidth,
                                    gint          destHeight,
                                    gint          destBPP,
                                    gboolean      destHasAlpha,
                                    GimpDrawable *sourceDrawable,
                                    gint          x0,
                                    gint          y0,
                                    gint          sourceWidth,
                                    gint          sourceHeight);
static void util_convertColorspace (guchar       *dest,
                                    gint          destBPP,
                                    gboolean      destHasAlpha,
                                    const guchar *source,
                                    gint          sourceBPP,
                                    gboolean      sourceHasAlpha,
                                    gint          length);

void dialogSourceChangedCallback(GimpIntComboBox *widget, PlugInVals *vals);
void dialogMaskChangedCallback(GtkWidget *widget, PlugInVals *vals);
void dialogStopPathChangedCallback(GtkWidget *widget, PlugInVals *vals);
void dialogThresholdChanged(GtkWidget *widget, PlugInVals *vals);

void maskTypeChangedCallback(GtkComboBox *widget, PlugInVals *vals);
void set_default_param(GtkWidget *widget);
void renderPreview(PlugInVals *vals);
void buildPreviewSourceImage (PlugInVals *vals);
void update_mask(PlugInVals *vals);
void update_image(PlugInVals *vals);
void update_stop_path(PlugInVals *vals);
void destroy();
void set_defaults(PlugInUIVals       *ui_vals);
void fill_stop_path_buffer_from_path(gint32 path_id);



/*  Public functions  */

gboolean dialog (
	PlugInVals         *vals,
	PlugInUIVals       *ui_vals)
{
  if (!gimp_drawable_is_valid(vals->image_drawable_id)) {
	  vals->image_drawable_id = default_vals.image_drawable_id;
  }
  if (!gimp_drawable_is_valid(vals->mask_drawable_id)) {
	  vals->mask_drawable_id = default_vals.mask_drawable_id;
  }
  if (!gimp_vectors_is_valid(vals->stop_path_id)) {
	  vals->stop_path_id = default_vals.stop_path_id;
  }
  vals->output_drawable_id = vals->image_drawable_id;

  print_vals(vals);
  set_defaults(ui_vals);
  interface_vals.imageID = gimp_drawable_get_image(vals->image_drawable_id);
  interface_vals.image_name = gimp_image_get_name(interface_vals.imageID);
  interface_vals.image_drawable = NULL;
  interface_vals.mask_drawable = NULL;
  if (vals->image_drawable_id >= 0) {
#ifdef DEBUG
	  g_warning("There is an input image drawable id");
#endif
	  interface_vals.image_drawable = gimp_drawable_get(vals->image_drawable_id);
  }
  if (vals->mask_drawable_id >= 0) {
	  interface_vals.mask_drawable = gimp_drawable_get(vals->mask_drawable_id);
  } else {
	  interface_vals.mask_drawable = NULL;
  }

  ui_state = ui_vals;

  //if there is a selection create mask drawable and fill

  gimp_drawable_mask_bounds(vals->image_drawable_id,&interface_vals.selectionX0,
		  	  	  	  	  	  	  	  	  	  	  	&interface_vals.selectionY0,
		  	  	  	  	  	  	  	  	  	  	  	&interface_vals.selectionX1,
		  	  	  	  	  	  	  	  		  	  	&interface_vals.selectionY1);
  interface_vals.selectionWidth = interface_vals.selectionX1-interface_vals.selectionX0;
  interface_vals.selectionHeight = interface_vals.selectionY1-interface_vals.selectionY0;

  gint image_width = gimp_drawable_width(vals->image_drawable_id);
  gint image_height = gimp_drawable_height(vals->image_drawable_id);

  interface_vals.selectionX0 -= PREVIEW_SIZE*0.1;
  if (interface_vals.selectionX0 < 0) interface_vals.selectionX0 = 0;
  interface_vals.selectionX1 += PREVIEW_SIZE*0.1;
  if (interface_vals.selectionX1 > image_width) interface_vals.selectionX1 = image_width;
  interface_vals.selectionY0 -= PREVIEW_SIZE*0.1;
  if (interface_vals.selectionY0 < 0) interface_vals.selectionY0 = 0;
  interface_vals.selectionY1 += PREVIEW_SIZE*0.1;
  if (interface_vals.selectionY1 > image_height) interface_vals.selectionY1 = image_height;

  interface_vals.selectionWidth = interface_vals.selectionX1-interface_vals.selectionX0;
  interface_vals.selectionHeight = interface_vals.selectionY1-interface_vals.selectionY0;

  //vals->mask_drawable_id = gimp_image_get_selection(gimp_drawable_get_image(vals->image_drawable_id));
  //g_warning("there is a selection with id = %d",vals->mask_drawable_id);
  //if (interface_vals.mask_drawable != NULL) gimp_drawable_detach(interface_vals.mask_drawable);
  //interface_vals.mask_drawable = gimp_drawable_get(vals->mask_drawable_id);


#ifdef DEBUG
  g_warning("image dims: x0,x1,y0,y1 = %d,%d,%d,%d",interface_vals.selectionX0,interface_vals.selectionX1,interface_vals.selectionY0,interface_vals.selectionY1);
#endif
  gchar text[100];
  sprintf(text,"Inpainting: %s",interface_vals.image_name);

  gimp_ui_init (PLUGIN_NAME, TRUE);

  GtkWidget* dlg = gimp_dialog_new (text, PLUGIN_NAME,
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-template",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,


			 NULL);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  GtkWidget* vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
		  vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* Preview */
  GtkWidget* hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  GtkWidget* frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

//  interface_vals.preview = TRUE;
//  interface_vals.preview_widget = GIMP_DRAWABLE_PREVIEW (gimp_drawable_preview_new(interface_vals.image_drawable,&interface_vals.preview));
//  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (interface_vals.preview_widget));
//  gtk_widget_show (GTK_WIDGET (interface_vals.preview_widget));

  interface_vals.previewWidth  = MIN (interface_vals.selectionWidth,  PREVIEW_SIZE);
  interface_vals.previewHeight = MIN (interface_vals.selectionHeight, PREVIEW_SIZE);
  interface_vals.preview_widget = gimp_preview_area_new ();
  gtk_widget_set_size_request (interface_vals.preview_widget,
		  interface_vals.previewWidth, interface_vals.previewHeight);
  gtk_container_add (GTK_CONTAINER (frame), interface_vals.preview_widget);
  gtk_widget_show (interface_vals.preview_widget);

  buildPreviewSourceImage (vals);


  /* Source and Mask selection */
  GtkWidget* table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  //gtk_table_set_row_spacing (GTK_TABLE (table), 1, 12);
  //gtk_table_set_row_spacing (GTK_TABLE (table), 3, 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  GtkWidget* label = gtk_label_new (_("Source:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		  GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  GtkWidget* combo = gimp_drawable_combo_box_new (NULL, NULL);
#ifdef DEBUG
  g_warning("setting initi value of source combo box as %d",vals->image_drawable_id);
#endif
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), vals->image_drawable_id,
		  G_CALLBACK (dialogSourceChangedCallback),vals);

  gtk_table_attach (GTK_TABLE (table), combo, 1, 3, 0, 1,
		  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (combo);

  label = gtk_label_new(_("Mask:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		  GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  interface_vals.mask_combo_widget = gimp_drawable_combo_box_new (NULL, NULL);
  if (interface_vals.mask_type == SELECTION) {
	  gtk_widget_set_sensitive(interface_vals.mask_combo_widget,FALSE);
  }
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (interface_vals.mask_combo_widget),
		  vals->mask_drawable_id,
  	  	  G_CALLBACK (dialogMaskChangedCallback),vals);


  gtk_table_attach (GTK_TABLE (table), interface_vals.mask_combo_widget, 1, 3, 1, 2,
		  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (interface_vals.mask_combo_widget);




  label = gtk_label_new (_("Stop Path:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		  GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  interface_vals.stop_path_combo_widget = gimp_vectors_combo_box_new (NULL, NULL);
  gtk_widget_set_sensitive(interface_vals.stop_path_combo_widget,FALSE);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (interface_vals.stop_path_combo_widget), vals->stop_path_id,
		  G_CALLBACK (dialogStopPathChangedCallback),vals);
  gtk_table_attach (GTK_TABLE (table), interface_vals.stop_path_combo_widget, 1, 3, 2, 3,
		  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (interface_vals.stop_path_combo_widget);


  label = gtk_label_new(_("Mask Type:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
		  GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  interface_vals.mask_type_widget = gtk_combo_box_new_text();
  gint num_vectors;
  gimp_image_get_vectors(interface_vals.imageID,&num_vectors);

  gtk_combo_box_append_text(GTK_COMBO_BOX(interface_vals.mask_type_widget),"Selection");

  if (num_vectors > 0)
	  gtk_combo_box_append_text(GTK_COMBO_BOX(interface_vals.mask_type_widget),"Selection With Stop Path");

  gtk_combo_box_append_text(GTK_COMBO_BOX(interface_vals.mask_type_widget),"Binary Mask");
  if (num_vectors > 0)
	  gtk_combo_box_append_text(GTK_COMBO_BOX(interface_vals.mask_type_widget),"Binary Mask With Stop Path");
  gtk_combo_box_append_text(GTK_COMBO_BOX(interface_vals.mask_type_widget),"Mask Including Ordering");

  if (interface_vals.mask_type == SELECTION) {
	  int mt_index = 0 + (vals->stop_path_id > 0);
	  gtk_combo_box_set_active(GTK_COMBO_BOX(interface_vals.mask_type_widget),mt_index);
  } else if (interface_vals.mask_type == BINARY_MASK) {
	  int mt_index = 1 + (num_vectors > 0) + (vals->stop_path_id > 0);
	  gtk_combo_box_set_active(GTK_COMBO_BOX(interface_vals.mask_type_widget),mt_index);
  } else {
	  int mt_index = 2 + 2*(num_vectors > 0);
	  gtk_combo_box_set_active(GTK_COMBO_BOX(interface_vals.mask_type_widget),mt_index);
  }

  g_signal_connect (interface_vals.mask_type_widget, "changed",
		  G_CALLBACK(maskTypeChangedCallback), vals);
  maskTypeChangedCallback(GTK_COMBO_BOX(interface_vals.mask_type_widget),vals);


  gtk_table_attach (GTK_TABLE (table), interface_vals.mask_type_widget, 1, 3, 3, 4,
		  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (interface_vals.mask_type_widget);

  // Create the parameter table
//  table = gtk_table_new (5, 3, FALSE);
//  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
//  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
//  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
//  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
//
//  gtk_widget_show (table);

  interface_vals.threshold_scale = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,"_Mask Threshold:", SCALE_WIDTH, 0,vals->threshold, 0, 255, 0.001, 0.1, EPS_DIGITS,TRUE, 0, 0,NULL, NULL);
   g_signal_connect (interface_vals.threshold_scale, "value_changed", 	G_CALLBACK(dialogThresholdChanged), vals);

   GtkWidget *separator = gtk_hseparator_new ();
   gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 5);
   gtk_widget_show (separator);

   table = gtk_table_new (5, 3, FALSE);
   gtk_table_set_col_spacings (GTK_TABLE (table), 6);
   gtk_table_set_row_spacings (GTK_TABLE (table), 6);
   //gtk_table_set_row_spacing (GTK_TABLE (table), 1, 12);
   //gtk_table_set_row_spacing (GTK_TABLE (table), 3, 12);
   gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
   gtk_widget_show (table);


  interface_vals.epsilon_scale = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,"_Pixel neighborhood (epsilon):", SCALE_WIDTH, 0,vals->epsilon, 1, SCALE_MAX, 0.001, 0.1, EPS_DIGITS,TRUE, 0, 0,NULL, NULL);
  g_signal_connect (interface_vals.epsilon_scale, "value_changed", 	G_CALLBACK(gimp_float_adjustment_update), &vals->epsilon);

  interface_vals.kappa_scale = gimp_scale_entry_new (GTK_TABLE (table), 0, 1, "_Sharpness (kappa in %):", SCALE_WIDTH, 0,	vals->kappa, 0, CONV_MAX, 0.001, 0.1, KAPPA_DIGITS,TRUE, 0, 0,NULL, NULL);
  g_signal_connect (interface_vals.kappa_scale, "value_changed", 	G_CALLBACK(gimp_float_adjustment_update), &vals->kappa);

  interface_vals.sigma_scale = gimp_scale_entry_new (GTK_TABLE (table), 0, 2, "_Pre-smoothing (sigma):", SCALE_WIDTH, 0,	vals->sigma, 0, SCALE_MAX, 0.001, 0.1, SMOOTH_DIGITS,TRUE, 0, 0,NULL, NULL);
  g_signal_connect (interface_vals.sigma_scale, "value_changed", 	G_CALLBACK(gimp_float_adjustment_update), &vals->sigma);

  interface_vals.rho_scale = gimp_scale_entry_new (GTK_TABLE (table), 0, 3, "_Post-smoothing (rho):", SCALE_WIDTH, 0,	 vals->rho, 0.001, SCALE_MAX, 0.001, 0.1, SMOOTH_DIGITS,TRUE, 0, 0,NULL, NULL);
  g_signal_connect (interface_vals.rho_scale, "value_changed", 	G_CALLBACK(gimp_float_adjustment_update), &vals->rho);


//  // test extra button
//  GtkWidget *togglebutton = gtk_check_button_new_with_label("Inpaint Animation");
//  gtk_toggle_button_set_active( (GtkToggleButton *) togglebutton, ui_vals->anim_mode);
//  gtk_widget_show(togglebutton);
//
//  gimp_table_attach_aligned(GTK_TABLE (table),0,4,NULL,0,0,togglebutton,1,TRUE);
//
//  g_signal_connect (togglebutton, "toggled",	G_CALLBACK(gimp_toggle_button_update), &ui_vals->anim_mode);

  GtkWidget *default_param_button =   gtk_button_new_with_label("Default Parameters");
  gtk_widget_show(default_param_button);
  gtk_table_attach((GtkTable *)table,default_param_button,0,1,4,5,GTK_EXPAND,GTK_EXPAND,0,0);
  g_signal_connect (default_param_button, "clicked",	G_CALLBACK(set_default_param), NULL);
  //test end

  // Display dialog
  gtk_widget_show(dlg);
  renderPreview(vals);

  GtkResponseType status = gimp_dialog_run (GIMP_DIALOG (dlg));

  while (status == GTK_RESPONSE_APPLY) {
	  render (vals);
	  gimp_displays_flush ();
	  status = gimp_dialog_run (GIMP_DIALOG (dlg));
  }
  ui_vals->mask_type = interface_vals.mask_type;
  destroy();
  gtk_widget_destroy (dlg);

  return (status == GTK_RESPONSE_OK);
}


/*  Private functions  */

static gboolean
dialog_image_constraint_func (gint32    image_id,
                              gpointer  data)
{
  return (gimp_image_base_type (image_id) == GIMP_RGB);
}

void dialogSourceChangedCallback(GimpIntComboBox *widget, PlugInVals *vals) {
	gint value;
#ifdef DEBUG
	g_warning("dialogSourceChangedCallback");
#endif

	if (gimp_int_combo_box_get_active(widget,&value)) {
		vals->image_drawable_id = value;
		vals->output_drawable_id = value;
		update_image(vals);
		renderPreview(vals);


	}
}
void dialogMaskChangedCallback(GtkWidget *widget, PlugInVals *vals) {
	gint value;
#ifdef DEBUG
	g_warning("dialogMaskChangedCallback");
#endif

	if (GTK_WIDGET_SENSITIVE(widget)) {
	//if (gtk_widget_get_sensitive(widget)) {
		if (gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(widget),&value)) {
			vals->mask_drawable_id = value;
		} else {
			vals->mask_drawable_id = -1;

		}
	} else {
		vals->mask_drawable_id = gimp_image_get_selection(gimp_drawable_get_image(vals->image_drawable_id));
	}
	update_mask(vals);
	renderPreview(vals);
}
void dialogStopPathChangedCallback(GtkWidget *widget, PlugInVals *vals) {
	gint value;
#ifdef DEBUG
	g_warning("dialogStopPathChangedCallback");
#endif
	if (GTK_WIDGET_SENSITIVE(widget)) {
	//if (gtk_widget_get_sensitive(widget)) {
		if (gimp_int_combo_box_get_active(GIMP_INT_COMBO_BOX(widget),&value)) {
			vals->stop_path_id = value;

		} else {
			vals->stop_path_id = -1;
		}
	} else {
		vals->stop_path_id = -1;
	}
	update_stop_path(vals);
	renderPreview(vals);
}


void dialogThresholdChanged(GtkWidget *widget, PlugInVals *vals) {
#ifdef DEBUG
	g_warning("old threshold = %d",vals->threshold);
#endif
	vals->threshold = gtk_adjustment_get_value(GTK_ADJUSTMENT(widget));
#ifdef DEBUG
	g_warning("new threshold = %d",vals->threshold);
#endif
	renderPreview(vals);
}


void maskTypeChangedCallback(GtkComboBox *widget, PlugInVals *vals) {
#ifdef DEBUG
	g_warning("maskTypeChangedCallback");
#endif
	if (g_ascii_strncasecmp(gtk_combo_box_get_active_text(widget),
							"Selection",9)==0) {
		gtk_widget_set_sensitive(interface_vals.mask_combo_widget,FALSE);
		interface_vals.mask_type = SELECTION;
		if (g_ascii_strncasecmp(gtk_combo_box_get_active_text(widget) + 9,
									" With Stop Path",15)==0) {
			gtk_widget_set_sensitive(interface_vals.stop_path_combo_widget,TRUE);
		} else {
			gtk_widget_set_sensitive(interface_vals.stop_path_combo_widget,FALSE);

		}
	} else {
		gtk_widget_set_sensitive(interface_vals.mask_combo_widget,TRUE);
		if (g_ascii_strncasecmp(gtk_combo_box_get_active_text(widget),
									"Binary Mask",11)==0) {
			interface_vals.mask_type = BINARY_MASK;
			if (g_ascii_strncasecmp(gtk_combo_box_get_active_text(widget) + 11,
					" With Stop Path",15)==0) {
				gtk_widget_set_sensitive(interface_vals.stop_path_combo_widget,TRUE);
			} else {
				gtk_widget_set_sensitive(interface_vals.stop_path_combo_widget,FALSE);
			}
		} else {
			interface_vals.mask_type = ORDER_MASK;
		}

	}

	dialogMaskChangedCallback(interface_vals.mask_combo_widget, vals);
	dialogStopPathChangedCallback(interface_vals.stop_path_combo_widget, vals);
}


void set_default_param(GtkWidget *widget)
{

	gtk_adjustment_set_value ((GtkAdjustment *)  interface_vals.epsilon_scale, default_vals.epsilon);
	gtk_adjustment_set_value ((GtkAdjustment *)  interface_vals.kappa_scale, default_vals.kappa);
	gtk_adjustment_set_value ((GtkAdjustment *)  interface_vals.sigma_scale,default_vals.sigma);
	gtk_adjustment_set_value ((GtkAdjustment *)  interface_vals.rho_scale, default_vals.rho);
}

void renderPreview(PlugInVals *vals) {
	gint    y,k,j;
	guchar *imageRow;
	guchar *resultRow;
	guchar *maskRow;
	guchar *stopRow;
	guchar *resultRGBA;
#ifdef DEBUG
	g_warning("renderPreview");
#endif

	resultRGBA = g_new (guchar, 4 * interface_vals.previewWidth *
			interface_vals.previewHeight);

	for (y = 0; y < interface_vals.previewHeight; y++)
	{
		imageRow =
				&(interface_vals.previewImage[  y * interface_vals.previewWidth * 4]);

		maskRow =
				&(interface_vals.previewMask[y * interface_vals.previewWidth  ]);

		stopRow =
				&(interface_vals.previewStopPath[y * interface_vals.previewWidth  ]);

		resultRow =
				&(resultRGBA[  y * interface_vals.previewWidth * 4]);

		for (k = 0, j=0; k < interface_vals.previewWidth; ++k,j+=4) {
			if (maskRow[k] > vals->threshold) {

				resultRow[j] = 0;
				resultRow[j+1] = 0;
				resultRow[j+2] = 0;
				if (stopRow[k] > 0) {
					resultRow[j+3] = 255;
				} else {
					resultRow[j+3] = 0;
				}
			} else {
				resultRow[j] = imageRow[j];
				resultRow[j+1] = imageRow[j+1];
				resultRow[j+2] = imageRow[j+2];
				resultRow[j+3] = imageRow[j+3];;
			}
		}
	}
	gimp_preview_area_draw (GIMP_PREVIEW_AREA (interface_vals.preview_widget),
			0, 0,
			interface_vals.previewWidth,
			interface_vals.previewHeight,
			GIMP_RGBA_IMAGE,
			resultRGBA,
			interface_vals.previewWidth * 4);
	g_free(resultRGBA);
}

void buildPreviewSourceImage (PlugInVals *vals) {
#ifdef DEBUG
	g_warning("buildPreviewSourceImage");
#endif

	interface_vals.previewImage   =
			g_new (guchar, interface_vals.previewWidth *
					interface_vals.previewHeight * 4);
	util_fillReducedBuffer (interface_vals.previewImage,
			interface_vals.previewWidth,
			interface_vals.previewHeight,
			4, TRUE,
			interface_vals.image_drawable,
			interface_vals.selectionX0, interface_vals.selectionY0,
			interface_vals.selectionWidth, interface_vals.selectionHeight);
	interface_vals.previewMask =
			g_new (guchar, interface_vals.previewWidth*interface_vals.previewHeight * 1);

	if (interface_vals.mask_drawable != NULL) {
		util_fillReducedBuffer (interface_vals.previewMask,
				interface_vals.previewWidth,
				interface_vals.previewHeight,
				1, FALSE,
				interface_vals.mask_drawable,
				interface_vals.selectionX0, interface_vals.selectionY0,
				interface_vals.selectionWidth, interface_vals.selectionHeight);
	}

	interface_vals.previewStopPath =
			g_new (guchar, interface_vals.previewWidth*interface_vals.previewHeight * 1);

	fill_stop_path_buffer_from_path(vals->stop_path_id);


//	else {
//		gint size = interface_vals.previewHeight*interface_vals.previewWidth;
//		gint i;
//		for (i = 0; i < size; ++i) {
//			interface_vals.previewMask[i] = 255;
//		}
//		//memset(interface_vals.previewMask,0,interface_vals.previewWidth*interface_vals.previewHeight);
//	}
	renderPreview(vals);
}

void destroy() {
	g_free(interface_vals.previewImage);
	g_free(interface_vals.previewMask);
	if (interface_vals.mask_drawable != NULL) gimp_drawable_detach(interface_vals.mask_drawable);
	if (interface_vals.image_drawable != NULL) gimp_drawable_detach(interface_vals.image_drawable);
}

void set_defaults(PlugInUIVals       *ui_vals) {
	interface_vals.epsilon_scale = NULL;
	interface_vals.imageID = 0;
	interface_vals.image_drawable = NULL;
	interface_vals.image_name = NULL;
	interface_vals.kappa_scale = NULL;
	interface_vals.mask_combo_widget = NULL;
	interface_vals.mask_drawable = NULL;
	interface_vals.mask_type_widget = NULL;
	interface_vals.mask_type = ui_vals->mask_type;
	interface_vals.preview = TRUE;
	interface_vals.previewHeight = PREVIEW_SIZE;
	interface_vals.previewImage = NULL;
	interface_vals.previewMask = NULL;
	interface_vals.previewResult = NULL;
	interface_vals.previewWidth = PREVIEW_SIZE;
	interface_vals.preview_widget = NULL;
	interface_vals.rho_scale = NULL;
	interface_vals.selectionHeight = 0;
	interface_vals.selectionWidth = 0;
	interface_vals.selectionX0 = 0;
	interface_vals.selectionX1 = 0;
	interface_vals.selectionY0 = 0;
	interface_vals.selectionY1 = 0;
	interface_vals.sigma_scale = NULL;
}

void update_mask(PlugInVals *vals) {
#ifdef DEBUG
	g_warning("update_mask to id = %d",vals->mask_drawable_id);
#endif
	if (interface_vals.mask_drawable != NULL) gimp_drawable_detach(interface_vals.mask_drawable);
	interface_vals.mask_drawable = gimp_drawable_get(vals->mask_drawable_id);

	util_fillReducedBuffer (interface_vals.previewMask,
			interface_vals.previewWidth,
			interface_vals.previewHeight,
			1, FALSE,
			interface_vals.mask_drawable,
			interface_vals.selectionX0, interface_vals.selectionY0,
			interface_vals.selectionWidth, interface_vals.selectionHeight);
}

void update_stop_path(PlugInVals *vals) {
#ifdef DEBUG
	g_warning("update_path to id = %d",vals->stop_path_id);
#endif
	fill_stop_path_buffer_from_path(vals->stop_path_id);
}

void update_image(PlugInVals *vals) {
#ifdef DEBUG
	g_warning("update_image to id = %d",vals->image_drawable_id);
#endif

	if (interface_vals.image_drawable != NULL) gimp_drawable_detach(interface_vals.image_drawable);
	interface_vals.image_drawable = gimp_drawable_get(vals->image_drawable_id);

	util_fillReducedBuffer (interface_vals.previewImage,
			interface_vals.previewWidth,
			interface_vals.previewHeight,
			4, TRUE,
			interface_vals.image_drawable,
			interface_vals.selectionX0, interface_vals.selectionY0,
			interface_vals.selectionWidth, interface_vals.selectionHeight);
}

void fill_stop_path_buffer_from_path(gint32 path_id) {
	int i,j,num_strokes;
	gint stroke_id;
#ifdef DEBUG
	g_warning("fill_stop_path_buffer_from_path with path_id = %d, previewWidth  = %d, height = %d",path_id,
			interface_vals.previewWidth,interface_vals.previewHeight );
#endif
	for (i = 0; i < interface_vals.previewWidth * interface_vals.previewHeight; i++)
			interface_vals.previewStopPath[i] = 0;
#ifdef DEBUG
	g_warning("fill_stop_path_buffer_from_path init finish",path_id);
#endif

	if (path_id <= 0) return;
	if (!gimp_vectors_is_valid(path_id)) {
		g_error("selected path is not valid!");
	}

	stroke_id = gimp_vectors_get_strokes(path_id,&num_strokes)[0];
#ifdef DEBUG
	g_warning("vectors has %d strokes, using stroke_id = %d",stroke_id);
#endif
	if (num_strokes > 0) {
		gboolean closed;
		gdouble   *coords;
		gint num_coords;
#ifdef DEBUG
		g_warning("going to interpolate");
#endif
		coords = gimp_vectors_stroke_interpolate(path_id,stroke_id,1.0,&num_coords,&closed);
#ifdef DEBUG
		g_warning("got %d interpolation points",num_coords/2);
#endif
		if (coords) {
			gint oldx,oldy;
			for (i = 0; i < num_coords; i+=2) {
				gint x = coords[i] - interface_vals.selectionX0;
				gint y = coords[i+1] - interface_vals.selectionY0;
#ifdef DEBUG
				g_warning("%d: putting in point x,y = %d,%d",i/2,x,y);
#endif

				if (x >= 0 && y >= 0 && x < interface_vals.previewWidth && y < interface_vals.previewHeight) {
					interface_vals.previewStopPath[y * interface_vals.previewWidth + x] = 1;
					if (i>0) {
						gdouble len = sqrt((gdouble)(x-oldx)*(x-oldx) + (gdouble)(y-oldy)*(y-oldy));
						if (len > 1) {
							gdouble xcoeff = (gdouble)(x-oldx) / len;
							gdouble ycoeff = (gdouble)(y-oldy) / len;
							for (j = 0; j < floor(len); ++j) {
								gint xtmp = ROUND(oldx + xcoeff*j);
								gint ytmp = ROUND(oldy + ycoeff*j);
								interface_vals.previewStopPath[ytmp * interface_vals.previewWidth + xtmp] = 1;
							}
						}
					}
				}
				oldx = x;
				oldy = y;
			}
			g_free(coords);
		}
	}
#ifdef DEBUG
	g_warning("fill_stop_path_buffer_from_path finished");
#endif

}


/* ----- Utility routines ----- */

static void
util_fillReducedBuffer (guchar       *dest,
                        gint          destWidth,
                        gint          destHeight,
                        gint          destBPP,
                        gboolean      destHasAlpha,
                        GimpDrawable *sourceDrawable,
                        gint          x0,
                        gint          y0,
                        gint          sourceWidth,
                        gint          sourceHeight)
{
  GimpPixelRgn  rgn;
  guchar       *sourceBuffer,    *reducedRowBuffer;
  guchar       *sourceBufferPos, *reducedRowBufferPos;
  guchar       *sourceBufferRow;
  gint          x, y, i, yPrime;
  gboolean      sourceHasAlpha;
  gint          sourceBpp;
  gint         *sourceRowOffsetLookup;

  if ((sourceDrawable == NULL) || (sourceWidth == 0) || (sourceHeight == 0))
    {
      for (x = 0; x < destWidth * destHeight * destBPP; x++)
        dest[x] = 0;
      return;
    }

  sourceBpp = sourceDrawable->bpp;

  sourceBuffer          = g_new (guchar, sourceWidth * sourceHeight * sourceBpp);
  reducedRowBuffer      = g_new (guchar, destWidth   * sourceBpp);
  sourceRowOffsetLookup = g_new (int, destWidth);
  gimp_pixel_rgn_init (&rgn, sourceDrawable,
                       x0, y0, sourceWidth, sourceHeight,
                       FALSE, FALSE);
  sourceHasAlpha = gimp_drawable_has_alpha (sourceDrawable->drawable_id);

  for (x = 0; x < destWidth; x++)
    sourceRowOffsetLookup[x] = (x * (sourceWidth - 1) / (destWidth - 1)) * sourceBpp;

  gimp_pixel_rgn_get_rect (&rgn, sourceBuffer,
                           x0, y0, sourceWidth, sourceHeight);

  for (y = 0; y < destHeight; y++)
    {
      yPrime = y * (sourceHeight - 1) / (destHeight - 1);
      sourceBufferRow = &(sourceBuffer[yPrime * sourceWidth * sourceBpp]);
      sourceBufferPos = sourceBufferRow;
      reducedRowBufferPos = reducedRowBuffer;
      for (x = 0; x < destWidth; x++)
        {
          sourceBufferPos = sourceBufferRow + sourceRowOffsetLookup[x];
          for (i = 0; i < sourceBpp; i++)
            reducedRowBufferPos[i] = sourceBufferPos[i];
          reducedRowBufferPos += sourceBpp;
        }
      util_convertColorspace(&(dest[y * destWidth * destBPP]), destBPP, destHasAlpha,
                             reducedRowBuffer, sourceDrawable->bpp, sourceHasAlpha,
                             destWidth);
    }

  g_free (sourceBuffer);
  g_free (reducedRowBuffer);
  g_free (sourceRowOffsetLookup);
}

/* Utterly pathetic kludge to convert between color spaces;
   likes gray and rgb best, of course.  Others will be creatively mutilated,
   and even rgb->gray is pretty bad */
static void
util_convertColorspace (guchar       *dest,
                        gint          destBPP,
                        gboolean      destHasAlpha,
                        const guchar *source,
                        gint          sourceBPP,
                        gboolean      sourceHasAlpha,
                        gint          length)
{
  gint i, j;
  gint sourcePos, destPos;
  gint accum;
  gint sourceColorBPP = sourceHasAlpha ? (sourceBPP - 1) : sourceBPP;
  gint destColorBPP   = destHasAlpha   ? (destBPP   - 1) : destBPP;

  if ((sourceColorBPP == destColorBPP) &&
      (sourceBPP      == destBPP     ))
    {
      j = length * sourceBPP;
      for (i = 0; i < j; i++)
        dest[i] = source[i];
      return;
    }

  if (sourceColorBPP == destColorBPP)
    {
      for (i = destPos = sourcePos = 0;
           i < length;
           i++, destPos += destBPP, sourcePos += sourceBPP)
        {
          for (j = 0; j < destColorBPP; j++)
            dest[destPos + j] = source[sourcePos + j];
        }
    }
  else if (sourceColorBPP == 1)
    {
      /* Duplicate single "gray" source byte across all dest bytes */
      for (i = destPos = sourcePos = 0;
           i < length;
           i++, destPos += destBPP, sourcePos += sourceBPP)
        {
          for (j = 0; j < destColorBPP; j++)
            dest[destPos + j] = source[sourcePos];
        }
    }
  else if (destColorBPP == 1)
    {
      /* Average all source bytes into single "gray" dest byte */
      for (i = destPos = sourcePos = 0;
           i < length;
           i++, destPos += destBPP, sourcePos += sourceBPP)
        {
          accum = 0;
          for (j = 0; j < sourceColorBPP; j++)
            accum += source[sourcePos + j];
          dest[destPos] = accum/sourceColorBPP;
        }
    }
  else if (destColorBPP < sourceColorBPP)
    {
      /* Copy as many corresponding bytes from source to dest as will fit */
      for (i = destPos = sourcePos = 0;
           i < length;
           i++, destPos += destBPP, sourcePos += sourceBPP)
        {
          for (j = 0; j < destColorBPP; j++)
            dest[destPos + j] = source[sourcePos + j];
        }
    }
  else /* destColorBPP > sourceColorBPP */
    {
      /* Fill extra dest bytes with zero */
      for (i = destPos = sourcePos = 0;
           i < length;
           i++, destPos += destBPP, sourcePos += sourceBPP)
        {
          for (j = 0; j < sourceColorBPP; j++)
            dest[destPos + j] = source[destPos + j];
          for (     ; j < destColorBPP; j++)
            dest[destPos + j] = 0;
        }
    }

  if (destHasAlpha)
    {
      if (sourceHasAlpha)
        {
          for (i = 0, destPos = destColorBPP, sourcePos = sourceColorBPP;
               i < length;
               i++, destPos += destBPP, sourcePos += sourceBPP)
            {
              dest[destPos] = source[sourcePos];
            }
        }
      else
        {
          for (i = 0, destPos = destColorBPP;
               i < length;
               i++, destPos += destBPP)
            {
              dest[destPos] = 255;
            }
        }
    }
}

