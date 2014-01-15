/* main.h  --- inpaintBCT
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

#ifndef __MAIN_H__
#define __MAIN_H__

#define MAX_CHANNELS 10

enum MaskType {SELECTION,BINARY_MASK,ORDER_MASK};


typedef struct
{
	gint32 image_drawable_id;
	gint32 mask_drawable_id;
	gint32 output_drawable_id;
	gint32 stop_path_id;
	gfloat epsilon;
	gfloat kappa;
	gfloat sigma;
	gfloat rho;
	guchar threshold;
	gboolean contains_ordering;
} PlugInVals;


typedef struct
{
  enum MaskType mask_type;
} PlugInUIVals;

void print_vals(PlugInVals *vals);


/*  Default values  */

extern const PlugInVals         default_vals;
extern const PlugInUIVals       default_ui_vals;


#endif /* __MAIN_H__ */
