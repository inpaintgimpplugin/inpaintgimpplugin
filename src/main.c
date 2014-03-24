/* main.c  --- inpaintBCT
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "main.h"
#include "interface.h"
#include "render.h"

#include "plugin-intl.h"


/*  Constants  */

#define PROCEDURE_NAME   "gimp_inpaint_BCT"

#define DATA_KEY_VALS    "gimp_inpaint_BCT"
#define DATA_KEY_UI_VALS "gimp_inpaint_BCT_ui"

#define PARASITE_KEY     "gimp_inpaint_BCT_options"


/*  Local function prototypes  */

static void   query (void);
static void   run   (const gchar      *name,
		     gint              nparams,
		     const GimpParam  *param,
		     gint             *nreturn_vals,
		     GimpParam       **return_vals);


/*  Local variables  */

const PlugInVals default_vals =
{
  -1,-1,-1,-1,
  5,
  25,
  1.41,
  4,
  127,
  FALSE
};

const PlugInUIVals default_ui_vals =
{
  0
};

static PlugInVals         vals;
static PlugInUIVals       ui_vals;


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
	gchar *help_path;
	gchar *help_uri;


	static GimpParamDef args[] =
	{
			{ GIMP_PDB_INT32,    "run_mode",   "Interactive, non-interactive"    },
			{ GIMP_PDB_IMAGE,    "image",      "Input image (not used)"          },
			{ GIMP_PDB_IMAGE,    "image drawable",   "Input image drawable"   },
			{ GIMP_PDB_DRAWABLE, "mask drawable",   "Input mask drawable"        },
			{ GIMP_PDB_DRAWABLE, "output drawable",   "Output mask drawable"        },
			{ GIMP_PDB_VECTORS, "stop path",   "stop path"        },
			{ GIMP_PDB_FLOAT,    "epsilon",      "epsilon"                       },
			{ GIMP_PDB_FLOAT,    "kappa",      "kappa"                          },
			{ GIMP_PDB_FLOAT,    "sigma",      "sigma"                          },
			{ GIMP_PDB_FLOAT,    "rho",       "rho" },
			{ GIMP_PDB_INT8,    "threshold",       "Mask Threshold" },
			{ GIMP_PDB_INT8,    "contains ordering",       "!= 0 if mask contains ordering" }
	};

	gimp_plugin_domain_register (PLUGIN_NAME, LOCALEDIR);

	help_path = g_build_filename (DATADIR, "help", NULL);
	help_uri = g_filename_to_uri (help_path, NULL, NULL);
	g_free (help_path);

	gimp_plugin_help_register ("http://inpaintgimpplugin.github.io/",
			help_uri);


	gimp_install_procedure (PROCEDURE_NAME,
			"Inpainting Based on Coherence Transport",
			"Inpainting Based on Coherence Transport",
			"Tom März <maerz@maths.ox.ac.uk> and Martin Robinson <martin.robinson@maths.ox.ac.uk>",
			"Copyright Tom März <maerz@maths.ox.ac.uk> and Martin Robinson <martin.robinson@maths.ox.ac.uk>",
			"2013",
			N_("_Inpainting..."),
			"RGB*, GRAY*, INDEXED*",
			GIMP_PLUGIN,
			G_N_ELEMENTS (args), 0,
			args, NULL);

	gimp_plugin_menu_register (PROCEDURE_NAME, "<Image>/Filters/Misc/");
}

static void
run (const gchar      *name,
		gint              n_params,
		const GimpParam  *param,
		gint             *nreturn_vals,
		GimpParam       **return_vals)
{
	static GimpParam   values[1];
	GimpRunMode        run_mode;
	GimpPDBStatusType  status = GIMP_PDB_SUCCESS;

	*nreturn_vals = 1;
	*return_vals  = values;

	/*  Initialize i18n support  */
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
	textdomain (GETTEXT_PACKAGE);

	run_mode = param[0].data.d_int32;

	/*  Initialize with default values  */
	vals          = default_vals;
	ui_vals       = default_ui_vals;
	gint32 imageID = param[1].data.d_image;
	vals.image_drawable_id = gimp_image_get_active_drawable(imageID);
	vals.output_drawable_id = vals.image_drawable_id;

	if (strcmp (name, PROCEDURE_NAME) == 0) {
		switch (run_mode) {
		case GIMP_RUN_NONINTERACTIVE:
			if (n_params != 12) {
				status = GIMP_PDB_CALLING_ERROR;
			}
			else {
				vals.image_drawable_id  = param[2].data.d_drawable;
				vals.mask_drawable_id   = param[3].data.d_drawable;
				vals.output_drawable_id = param[4].data.d_drawable;
				vals.stop_path_id		= param[5].data.d_vectors;
				vals.epsilon            = param[6].data.d_float;
				vals.kappa              = param[7].data.d_float;
				vals.sigma              = param[8].data.d_float;
				vals.rho                = param[9].data.d_float;
				vals.threshold		    = param[10].data.d_int8;
				vals.contains_ordering  = param[11].data.d_int8 != 0;
			}
			break;

		case GIMP_RUN_INTERACTIVE:
			/*  Possibly retrieve data  */

			gimp_get_data (DATA_KEY_VALS,    &vals);
			gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
			vals.image_drawable_id = gimp_image_get_active_drawable(imageID);


			if (!dialog (&vals, &ui_vals)) {
				status = GIMP_PDB_CANCEL;
			}
			break;

		case GIMP_RUN_WITH_LAST_VALS:
			/*  Possibly retrieve data  */
			gimp_get_data (DATA_KEY_VALS, &vals);
			gimp_get_data (DATA_KEY_UI_VALS, &ui_vals);
			break;

		default:
			break;
		}
	}
	else
	{
		status = GIMP_PDB_CALLING_ERROR;
	}

	if (status == GIMP_PDB_SUCCESS)
	{
		render (&vals);

		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush ();

		if (run_mode == GIMP_RUN_INTERACTIVE)
		{
			gimp_set_data (DATA_KEY_VALS,    &vals,    sizeof (vals));
			gimp_set_data (DATA_KEY_UI_VALS, &ui_vals, sizeof (ui_vals));
		}

	}

	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = status;
}

void print_vals(PlugInVals *vals) {
#ifdef DEBUG
	g_warning("vals.image_drawable_id = %d\n"
			   "vals.mask_drawable_id = %d\n"
			   "vals.output_drawable_id = %d\n"
			"vals.stop_path_id = %d\n"
			"vals.epsilon = %f\n"
			"vals.kappa = %f\n"
			"vals.sigma = %f\n"
			"vals.rho = %f\n", vals->image_drawable_id, vals->mask_drawable_id, vals->output_drawable_id,
			vals->stop_path_id,
			vals->epsilon, vals->kappa, vals->sigma, vals->rho);
#endif
}
