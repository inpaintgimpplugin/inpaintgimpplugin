/* render.cpp  --- inpaintBCT
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
#include <stdlib.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
extern "C" {
#include "main.h"
}
#include "render.h"
#include "inpainting_func.h"

#include <Eigen/Core>
#include <Eigen/Sparse>

#include <algorithm>



#include "plugin-intl.h"

#define NO_ERR 0
#define ERR_NARGIN 1
#define ERR_DOUBLE 2
#define ERR_STRING 3
#define ERR_DIM 4

#define ERR_PARAM_VAL_N 5
#define ERR_PARAM_VAL_C 6
#define ERR_PARAM_DIM_N 7
#define ERR_PARAM_DIM_C 8
#define ERR_NO_MASK_ORDER 9
#define ERR_COEFF 10
#define ERR_ORDER 11
#define ERR_ARG_MISSING 12
#define ERR_UNKNOWN_ID 13
#define ERR_MASK_DIM 14
#define ERR_EMPTY_MASK 15
#define ERR_NO_MASK 16
#define ERR_VECTORS_NOT_VALID 17
#define ERR_DECOMPOSITION_FAILED 18
#define ERR_SOLVING_FAILED 19
#define ERR_INVALID_IMAGE_ID 20
#define ERR_PATH_OUTSIDE_MASK 21


#define TYPE_N 0
#define TYPE_C 1
#define TYPE_D 2

void *AllocMem(size_t n)
{
	void *p;
	p = malloc( n );
	return p;
}

void FreeMem(void* p)
{
	free(p);
}

void ErrorMessage(int type)
{
#ifdef DEBUG
	g_warning("error with id = %d",type);
#endif
    switch( type )
    {
       	case ERR_INVALID_IMAGE_ID:
    		g_message("Error: the input source image id is not valid\n");
    		break;
       	case ERR_PATH_OUTSIDE_MASK:
       		g_message("Error: the given path is outside the domain to be inpainted\n");
       		break;
    	case ERR_VECTORS_NOT_VALID:
    	    g_message("Error: the input path id is not valid\n");
    	    break;
    	case ERR_DECOMPOSITION_FAILED:
    		g_message("Error: decomposition for solution of Laplace failed (in calculation of order from path)\n");
    		break;
    	case ERR_SOLVING_FAILED:
    		g_message("Error: solving of Laplace failed (in calculation of order from path)\n");
    		break;
        case ERR_NARGIN:
            g_message("Error: The number of input arguments must be odd and greater than 2! \n");
            break;

        case ERR_DOUBLE:
            g_message("Error: Data type of all numeric arguments must be double \n");
            break;

        case ERR_DIM:
            g_message("Error: Image must be n x m matrix or n x m x k multi-matricx for k-channel images \n");
            break;

        case ERR_MASK_DIM:
            g_message("Error: Mask <-> Image dimension mismatch \n");
            break;

        case ERR_NO_MASK:
            g_message("Error: the input mask id is not valid\n");
            break;

        case ERR_EMPTY_MASK:
            g_message("Error: Empty mask, nothing to do \n");
            break;

        case ERR_ARG_MISSING:
            g_message("Error: Argument missing \n");
            break;

        case ERR_UNKNOWN_ID:
            g_message("Error: Unknown specifier \n");
            break;

        case ERR_STRING:
            g_message("Error: All argument specifiers must be strings \n");
            break;

        case ERR_PARAM_VAL_N:
            g_message("Epsilon must be greater than or equal 1. \n");
			break;

        case ERR_PARAM_VAL_C:
            g_message("Error in the parameter values: \n");
            g_message("Epsilon must be greater than or equal 1. \n");
            g_message("The parameters kappa and sigma must be greater than or equal zero. \n");
			g_message("Rho must be greater than zero \n");
			g_message("Thresh must be greater than or equal zero \n");
            g_message("Quant must be greater than zero \n");
			break;

        case ERR_PARAM_DIM_N:
            g_message("Error: Parameter must be a scalar \n");
            break;

        case ERR_PARAM_DIM_C:
            g_message("Error: Parameter list must be a vector \n");
            break;

        case ERR_COEFF:
            g_message("Error in the channel coefficients: \n");
            g_message("The channel coefficients must be a vector and \n");
            g_message("the dimension of the coefficients vector must match the number of channels. \n");
            g_message("Any coefficient must be greater than or equal zero and the sum of the coefficients must be greater than zero. \n");
            break;

        case ERR_NO_MASK_ORDER:
            g_message("Error: Mask has to be specified, there is no default for this parameter. \n");
            break;


        case ERR_ORDER:
            g_message("Error: The combined mask and order data M must be a 3 x N matrix, \n");
            g_message("       where M(:,k) = [i(k) ; j(k) ; T(i,j)] and N is the number of points to be inpainted. \n");
            break;
        default:
        	g_message("Error: default error message\n");
    }
}

void SetDefaults(Data *data)
{
    data->rows = 1;
    data->cols = 1;
    data->channels = 1;
    data->size = 1;
    data->Image = NULL;
    data->MImage = NULL;



    // default parameters
    data->epsilon = 5;
    data->radius = 5;
    data->kappa = 25;
    data->sigma = 1.414213562373095; // sqrt(2.0);
    data->rho = 5;
    data->thresh = 0;
    data->delta_quant4 = 1; // default quantization range is [0,255]

    data->convex = NULL;


    data->ordered_points = NULL;
    data->nof_points2inpaint = 0;
    data->heap = NULL;
    data->Tfield = NULL;
    data->Domain = NULL;
    data->MDomain = NULL;
    data->GivenGuidanceT = NULL;

    data->lenSK1 = 0;
    data->lenSK2 = 0;
    data->SKernel1 = NULL;
    data->SKernel2 = NULL;
    data->Ihelp = NULL;
    data->Shelp = NULL;

    data->ordergiven = 0;
    data->guidance = 1;

    data->inpaint_undefined = 0;
}

void ClearMemory(Data *data)
{
	if( data->Image != NULL )
	{
		FreeMem( data->Image );
		data->Image = NULL;
	}

	if( data->MImage != NULL )
	{
		FreeMem( data->MImage );
		data->MImage = NULL;
	}

    if( data->Domain != NULL )
    {
        FreeMem( data->Domain );
        data->Domain = NULL;
    }

    if( data->MDomain != NULL )
    {
        FreeMem( data->MDomain );
        data->MDomain = NULL;
    }

    if( data->Tfield != NULL )
    {
        FreeMem( data->Tfield );
        data->Tfield = NULL;
    }

    if( data->heap != NULL )
    {
        FreeMem( data->heap );
        data->heap = NULL;
    }

    if( data->Ihelp != NULL )
    {
        FreeMem( data->Ihelp );
        data->Ihelp = NULL;
    }

    if( data->convex != NULL )
    {
        FreeMem( data->convex );
        data->convex = NULL;
    }

    if( data->SKernel1 != NULL )
    {
        FreeMem( data->SKernel1 );
        data->SKernel1 = NULL;
    }

    if( data->SKernel2 != NULL )
    {
        FreeMem( data->SKernel2 );
        data->SKernel2 = NULL;
    }

    if( data->Shelp != NULL )
    {
        FreeMem( data->Shelp );
        data->Shelp = NULL;
    }

    if( data->ordered_points != NULL )
    {
        FreeMem( data->ordered_points );
        data->ordered_points = NULL;
    }

    if( data->inpaint_index != NULL )
    {
    	FreeMem( data->inpaint_index );
    	data->inpaint_index = NULL;
    }
}

void display_preview(Data *data) {
	GtkWidget* dlg = gimp_dialog_new ("tmp tmp", "tmp",
			NULL, GTK_DIALOG_MODAL,
			gimp_standard_help_func, "plug-in-template",
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK,     GTK_RESPONSE_OK,
			NULL);

	GtkWidget* vbox = gtk_vbox_new (TRUE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
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


	GtkWidget* preview = gimp_preview_area_new ();
	gtk_widget_set_size_request (preview,
			data->cols, data->rows);
	gtk_container_add (GTK_CONTAINER (frame), preview);
	gtk_widget_show (preview);
	guchar* buffer = g_new (guchar, data->cols*data->rows*2);
	for( int j = 0; j < data->cols ; j++) {
		for( int i = 0; i < data->rows ; i++) {
			const int index = j*data->rows + i;
			const int bindex = i*data->cols + j;
			buffer[bindex*2] = data->Tfield[index].T;
			buffer[bindex*2+1] = (data->Domain[index]==0)*255;
		}
	}

	gtk_widget_show(dlg);
	gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
				0, 0,
				data->cols,
				data->rows,
				GIMP_GRAYA_IMAGE,
				buffer,
				data->cols*2);

	gboolean run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

	gtk_widget_destroy (dlg);
	g_free(buffer);
}


int SetImageAndMask(GimpDrawable *image, GimpDrawable *mask, Data *data) {

	GimpPixelRgn region_out;		// region of interest in drawable, write only
	gimp_pixel_rgn_init(&region_out, image, data->xmin,data->ymin,data->cols,data->rows,TRUE ,TRUE);

	// alloc memory for the buffers
	gint image_channels = gimp_drawable_bpp(image->drawable_id);
	guchar *pixel_out = g_new(guchar,image_channels * data->cols);

	// write result back
	for( int y = data->ymin, i=0 ; y < data->ymax ; y++, i++) {
		if (i%10==0) gimp_progress_update(0.9+0.1*(gdouble)i/(gdouble)(data->ymax-data->ymin));

		for(int c = 0 ; c < data->channels ; c++) {
			for(int j=0 , k=0 ; j < data->cols ; j++ , k+=image_channels) {
				int index = c*data->size + j*data->rows + i;
				pixel_out[k + c] = (guchar) (data->Image[index]);
			}
		}
		gimp_pixel_rgn_set_row(&region_out,pixel_out,data->xmin,y,data->cols);
	}

	// dealloc memory
	g_free(pixel_out);

	gimp_drawable_flush(image);
	gimp_drawable_merge_shadow(image->drawable_id, TRUE);
	gimp_drawable_update(image->drawable_id,data->xmin,data->ymin,data->cols,data->rows);

}


int GetImageAndMask( GimpDrawable *image, GimpDrawable *mask, Data *data)
{
    int err = NO_ERR;
    int nof_dim;
    double *parg;
    int i,j,c,y;
    int index;
    int not_equal;



    gint image_channels = gimp_drawable_bpp(image->drawable_id);
	gint mask_channels = gimp_drawable_bpp(mask->drawable_id);

    if (gimp_drawable_has_alpha(image->drawable_id)) {
    	data->channels = image_channels-1;
    } else {
    	data->channels = image_channels;
    }
    data->convex = (double *)AllocMem(sizeof(double)*data->channels);
    for (int i = 0; i < data->channels; ++i) {
    	data->convex[i] = 100.0/data->channels;
    }
    //g_message("epsilon %f kappa %f sigma %f rho %f delta_quant4 %f convex[0] %f channels %d",data->epsilon,data->kappa,data->sigma,data->rho,data->delta_quant4,data->convex[0], data->channels);
#ifdef DEBUG
    g_warning("convex = %f %f %f", data->convex[0], data->convex[1],data->convex[2]);
#endif



    //g_message("xmin %d xmax %d ymin %d ymax %d n_extrachannels %d maskaddress %d drawad %d",data->xmin,data->xmax,data->ymin,data->ymax,data->channels,mask->drawable_id,image->drawable_id);

    data->Image = (double *)AllocMem(sizeof(double) * data->size * data->channels);
    data->MImage = (double *)AllocMem(sizeof(double) * data->size * data->channels);

    data->Ihelp = (double *)AllocMem(sizeof(double) * data->channels);
    data->Tfield = (hItem *) AllocMem(sizeof(hItem) * data->size);
    data->Domain = (double *) AllocMem(sizeof(double) * data->size);
    data->MDomain = (double *) AllocMem(sizeof(double) * data->size);
    data->heap = (hItem **) AllocMem(sizeof(hItem *) * data->size);
    data->ordered_points = (double *) AllocMem(sizeof(double) * data->size *3);
    data->inpaint_index = (int *) AllocMem(sizeof(int) * data->size);


    data->nof_points2inpaint = 0;


    GimpPixelRgn region;				// region of interest in drawable, read only
    GimpPixelRgn mregion;			// region of interest in mask, read only
    gimp_pixel_rgn_init(&region, image, data->xmin,data->ymin,data->cols,data->rows,0 ,0);
    gimp_pixel_rgn_init(&mregion, mask, data->xmin,data->ymin,data->cols,data->rows,0 ,0);

    // alloc memory for the buffers
    guchar *pixel = g_new(guchar,image_channels * data->cols);
    guchar *mpixel = g_new(guchar, mask_channels * data->cols);

    // make a float copy of drawable and mask
    for( y = data->ymin, i=0 ; y < data->ymax ; y++, i++)
    {
    	if (i%10==0) gimp_progress_update(0.1*(gdouble)i/(gdouble)(data->ymax-data->ymin));
    	gimp_pixel_rgn_get_row(&region,pixel,data->xmin,y,data->cols);
    	gimp_pixel_rgn_get_row(&mregion,mpixel,data->xmin,y,data->cols);

    	for(gint c = 0 ; c < data->channels ; c++) {
    		for( int j=0, k=0, l=0 ; j < data->cols ; j++ , k+=image_channels, l+=mask_channels) {
    			index = j * data->rows + i;
    			if( mpixel[l] ) {//INSIDE
    				if( c == 0 ) {
    					data->ordered_points[data->nof_points2inpaint*3] = i;
    					data->ordered_points[data->nof_points2inpaint*3+1] = j;
    					data->ordered_points[data->nof_points2inpaint*3+2] = -1;
    					data->inpaint_index[index] = data->nof_points2inpaint;
    					data->nof_points2inpaint = data->nof_points2inpaint + 1;
    					data->Domain[index] = 0;
    					data->MDomain[index] = 0;
    				}
    				data->Image[index+c*data->size] = 0;
					data->MImage[index+c*data->size] = 0;
    			} else {// OUTSIDE
    				if( c == 0 ) {
    					data->Domain[index] = 1;
    					data->MDomain[index] = 1;
    				}
    				data->Image[index+c*data->size] = (double) (pixel[c + k]);
					data->MImage[index+c*data->size] = data->Image[index+c*data->size];

    			}
    		}
    	}
    }


    if( data->nof_points2inpaint == 0 )
    	err = ERR_EMPTY_MASK;

    g_free(pixel);
    g_free(mpixel);

    return err;
}

typedef std::pair<double,std::pair<double,double> > mytuple;
bool comparator ( const mytuple& l, const mytuple& r) {
	return l.first < r.first;
}
int CalculateOrderFromPath( gint32 vectors_id, Data *data) {
	if (!gimp_vectors_is_valid(vectors_id)) return ERR_VECTORS_NOT_VALID;

	//init Tfield

	for( int j = 0; j < data->cols ; j++) {
		for( int i = 0; i < data->rows ; i++) {
			int index = j*data->rows + i;
			if (data->Domain[index] == 1) {
				//outside
				data->Tfield[index].T = -1;
				data->Tfield[index].flag = KNOWN;
				data->Tfield[index].hpos = -1;
				data->Tfield[index].i = i;
				data->Tfield[index].j = j;
			} else {
				//inside
				data->Tfield[index].T = -1;
				data->Tfield[index].flag = TO_INPAINT;
				data->Tfield[index].hpos = -1;
				data->Tfield[index].i = i;
				data->Tfield[index].j = j;
			}
		}
	}

	// calculate end pixels from path
	int i,j,num_strokes;
	gint stroke_id;
#ifdef DEBUG
	g_warning("fill_stop_path_buffer_from_path with path_id = %d",vectors_id);
#endif

	if (!gimp_vectors_is_valid(vectors_id)) {
		return ERR_VECTORS_NOT_VALID;
	}

	stroke_id = gimp_vectors_get_strokes(vectors_id,&num_strokes)[0];
#ifdef DEBUG
	g_warning("vectors has %d strokes, using stroke_id = %d",stroke_id);
#endif
	if (num_strokes > 0) {
		gboolean closed;
		gdouble  *coords;
		gint num_coords;
#ifdef DEBUG
		g_warning("going to interpolate");
#endif
		coords = gimp_vectors_stroke_interpolate(vectors_id,stroke_id,1.0,&num_coords,&closed);
#ifdef DEBUG
		g_warning("got %d interpolation points",num_coords/2);
#endif
		if (coords) {
			gint oldx,oldy;
			for (i = 0; i < num_coords; i+=2) {
				gint x = coords[i] - data->xmin;
				gint y = coords[i+1] - data->ymin;


				if (x >= 0 && y >= 0 && x < data->cols && y < data->rows) {
					const int index = x*data->rows + y;
					if (data->Domain[index] != 0) return ERR_PATH_OUTSIDE_MASK;
					data->Tfield[index].T = 255;
					if (i>0) {
						gdouble len = sqrt((gdouble)(x-oldx)*(x-oldx) + (gdouble)(y-oldy)*(y-oldy));
						if (len > 1) {
							gdouble xcoeff = (gdouble)(x-oldx) / len;
							gdouble ycoeff = (gdouble)(y-oldy) / len;
							for (j = 0; j < floor(len); ++j) {
								gint xtmp = ROUND(oldx + xcoeff*j);
								gint ytmp = ROUND(oldy + ycoeff*j);
								data->Tfield[xtmp*data->rows + ytmp].T = 255;
							}
						}
					}
				} else {
					return ERR_PATH_OUTSIDE_MASK;
				}
				oldx = x;
				oldy = y;
			}
			g_free(coords);
		}
	}

	//construct Ax = b for laplace eq
	int n = 0;
	int *bindex_lookup = (int *) AllocMem(sizeof(int) * data->nof_points2inpaint);
	for (int i = 0,j = 0; j < data->nof_points2inpaint; i+=3,j++) {
		int index = data->ordered_points[i+1] * data->rows + data->ordered_points[i];
		if (data->Tfield[index].T == -1) {
			bindex_lookup[j] = n;
			n++;
		} else {
			bindex_lookup[j] = -1;
		}
	}
	Eigen::SparseMatrix<double> A(n,n);
	Eigen::VectorXd b(n);
	Eigen::VectorXd x(n);
	b.setZero();
	x.setZero();
#ifdef DEBUG
	g_warning("init of matricies finished");
#endif


	// transfer Tfield to A and b
	typedef Eigen::Triplet<double> T;
	std::vector<T> tripletList;
	tripletList.reserve(n*5);

	int bindex = 0;
	for (int i = 0,j = 0; j < data->nof_points2inpaint; i+=3,j++) {
		int index = data->ordered_points[i+1] * data->rows + data->ordered_points[i];
		if (data->Tfield[index].T == -1) {
			int nindex_list[4];
			nindex_list[0] = index - data->rows;
			nindex_list[1] = index - 1;
			nindex_list[2] = index + 1;
			nindex_list[3] = index + data->rows;
			for (int n = 0; n < 4; ++n) {
				int nindex = nindex_list[n];
				if (data->Domain[nindex]==1) {
					//neighbour is outside domain
					//b[bindex]--;
				} else if (data->Tfield[nindex].T != -1) {
					//neighbour is on stop path
					b[bindex] -= data->Tfield[nindex].T;
				} else {
					tripletList.push_back(T(bindex,bindex_lookup[data->inpaint_index[nindex]],1));
					//A.insert(bindex, bindex_lookup[data->inpaint_index[nindex]]) = 1;
				}
			}
			//A.insert(bindex, bindex) = -4;
			tripletList.push_back(T(bindex,bindex,-4));
			bindex++;
		}
	}
	A.setFromTriplets(tripletList.begin(), tripletList.end());
	//A.finalize();

#ifdef DEBUG
	g_warning("transfer tfield finished");
#endif
	//solve Ax = b
	Eigen::SimplicialLDLT<Eigen::SparseMatrix<double> > solver;
	solver.compute(A);
	if(solver.info() != Eigen::Success) {
		return ERR_DECOMPOSITION_FAILED;
	}
	x = solver.solve(b);
	if(solver.info() != Eigen::Success) {
		// solving failed
		return ERR_SOLVING_FAILED;
	}
#ifdef DEBUG
	g_warning("solving finished");
#endif



	//put result back in Tfield and ordered_points
	for (int i = 0,j = 0; j < data->nof_points2inpaint; i+=3,j++) {
        int index = data->ordered_points[i+1] * data->rows + data->ordered_points[i];
        if (data->Tfield[index].T == -1) {
        	data->Tfield[index].T = x[bindex_lookup[j]];
        }
        data->ordered_points[i+2] = data->Tfield[index].T;
	}

	//sort points
	std::vector<mytuple> ord_pt;
	ord_pt.reserve(data->nof_points2inpaint);
	for (int i = 0,j = 0; j < data->nof_points2inpaint; i+=3,j++) {
		ord_pt.push_back(mytuple(data->ordered_points[i+2],
				std::pair<double,double>(data->ordered_points[i],data->ordered_points[i+1])));
	}
	std::sort(ord_pt.begin(),ord_pt.end(),comparator);
	for (int i = 0,j = 0; j < data->nof_points2inpaint; i+=3,j++) {
		data->ordered_points[i] = ord_pt[j].second.first;
		data->ordered_points[i+1] = ord_pt[j].second.second;
		data->ordered_points[i+2] = ord_pt[j].first;
	}

	FreeMem(bindex_lookup);
	return NO_ERR;
}

//int GetOrder( const mxArray *arg, Data *data)
//{
//    int err = NO_ERR;
//    int rows;
//    int size;
//    double *parg;
//    int i,j,c;
//    int index;
//
//    if( !mxIsDouble(arg) )
//    {
//        err = ERR_DOUBLE;
//        return err;
//    }
//
//    rows = mxGetM( arg );
//    if( rows != 3)
//    {
//        err = ERR_ORDER;
//        return err;
//    }
//
//    // transfer info about ordered points
//    data->nof_points2inpaint = mxGetN( arg );
//
//    size = 3 * data->nof_points2inpaint;
//    parg = mxGetPr( arg );
//
//    for( i=0 ; i < size ; i++ )
//        if( (i+1) % 3 != 0 )
//            data->ordered_points[i] = parg[i] - 1;
//        else
//            data->ordered_points[i] = parg[i];
//
//    // transfer Tfield
//    for( j = 0; j < data->cols ; j++)
//    {
//        for( i = 0; i < data->rows ; i++)
//        {
//            index = j*data->rows + i;
//
//            // OUTSIDE
//            data->Domain[index] = 1;
//            data->MDomain[index] = 1;
//
//            data->Tfield[index].T = -1;
//            data->Tfield[index].flag = KNOWN;
//            data->Tfield[index].hpos = -1;
//            data->Tfield[index].i = i;
//            data->Tfield[index].j = j;
//        }
//    }
//
//    for( i=0 ; i < size ; i=i+3 )
//    {
//        index = data->ordered_points[i+1] * data->rows + data->ordered_points[i];
//
//        // INSIDE
//        data->Domain[index] = 0;
//        data->MDomain[index] = 0;
//
//        for(c = 0; c < data->channels ; c++)
//        {
//            data->Image[index + c * data->size] = 0;
//            data->MImage[index + c * data->size] = 0;
//        }
//
//        data->Tfield[index].T = data->ordered_points[i+2];
//        data->Tfield[index].flag = TO_INPAINT;
//        data->Tfield[index].hpos = -1;
//        data->Tfield[index].i = data->ordered_points[i];
//        data->Tfield[index].j = data->ordered_points[i+1];
//    }
//
//    return err;
//}

void SetKernels(Data *data)
{
    int i;
	int s;
	int r;

	s = std::max( int(round(2 * data->sigma)) , 1 );
	r = std::max( int(round(2 * data->rho)) , 1 );
	data->lenSK1 = 2*s +1;
	data->lenSK2 = 2*r +1;


    if( data->sigma > 0 )
    {
        data->SKernel1 = (double *)AllocMem(sizeof(double) * data->lenSK1);
        for( i=0 ; i < data->lenSK1 ; i++)
            data->SKernel1[i] = exp( -((i-s)*(i-s))/(2* data->sigma * data->sigma) );

        data->Shelp = (double *) AllocMem(sizeof(double) * data->lenSK1);
    }

    data->SKernel2 = (double *)AllocMem(sizeof(double) * data->lenSK2);
    for( i=0 ; i < data->lenSK2 ; i++)
        data->SKernel2[i] = exp( -((i-r)*(i-r))/(2* data->rho * data->rho) );

}

int CheckForValidInputs(PlugInVals* vals) {
	bool error = 0;
	if (!gimp_drawable_is_valid(vals->image_drawable_id)) {
		ErrorMessage(ERR_INVALID_IMAGE_ID);
		error = 1;
	}
	if (!gimp_drawable_is_valid(vals->mask_drawable_id)) {
		ErrorMessage(ERR_NO_MASK);
		error = 1;
	}
	if ((vals->stop_path_id != -1) && (!gimp_vectors_is_valid(vals->stop_path_id))) {
		ErrorMessage(ERR_VECTORS_NOT_VALID);
		error = 1;
	}
	return error;
}

int GetParam( const PlugInVals *vals, Data *data , int type)
{
    int err = NO_ERR;

    if( type == TYPE_C) {

    	data->epsilon = vals->epsilon;
    	data->radius = (int) (data->epsilon + 0.5);

    	data->kappa = vals->kappa;
    	data->sigma = vals->sigma;

    	data->rho = vals->rho;

    	//data->thresh = ??;

    	data->delta_quant4 = 1;
    	data->delta_quant4 = data->delta_quant4 * data->delta_quant4;
    	data->delta_quant4 = data->delta_quant4 * data->delta_quant4;

    	// check parameters
    	if( (data->epsilon < 1 ) || ( data->kappa < 0 ) || ( data->sigma < 0) || ( data->rho <= 0 ) || ( data->thresh < 0) || (data->delta_quant4 == 0) )
    	{
    		err = ERR_PARAM_VAL_C;
    		return err;
    	}

    }

    return err;
}





/*  Public functions  */

void
render (PlugInVals *vals) {
	print_vals(vals);
	Data data;

	int err = CheckForValidInputs(vals);
	if (err) return;

	// set default values
	SetDefaults(&data);

	//g_message("xmin %d xmax %d ymin %d ymax %d n_extrachannels %d maskaddress %d drawad %d",xmin,xmax,ymin,ymax,n_extrachannels,mask->drawable_id,drawable->drawable_id);


	err = GetParam( vals, &data, TYPE_C);
	data.guidance = 1;
	if( err ) {
		ErrorMessage(err);
		return;
	}

	GimpDrawable *image,*mask,*output;
	image = gimp_drawable_get(vals->image_drawable_id);
	mask = gimp_drawable_get(vals->mask_drawable_id);
	if (vals->output_drawable_id != vals->image_drawable_id) {
		output = gimp_drawable_get(vals->output_drawable_id);
	} else {
		output = image;
	}

	gimp_drawable_mask_bounds(image->drawable_id, &data.xmin, &data.ymin, &data.xmax, &data.ymax);
	data.rows = data.ymax - data.ymin;
	data.cols = data.xmax - data.xmin;
	gint image_width = gimp_drawable_width(vals->image_drawable_id);
	gint image_height = gimp_drawable_height(vals->image_drawable_id);
	if (data.cols < image_width) {
		data.xmin -= vals->epsilon + 1;
		if (data.xmin < 0) data.xmin = 0;
		data.xmax += vals->epsilon + 1;
		if (data.xmax > image_width) data.xmax = image_width;
		data.cols = data.xmax - data.xmin;
	}

	if (data.rows < image_height) {
		data.ymin -= vals->epsilon + 1;
		if (data.ymin < 0) data.ymin = 0;
		data.ymax += vals->epsilon + 1;
		if (data.ymax > image_height) data.ymax = image_height;
		data.rows = data.ymax - data.ymin;
	}
	data.size = data.rows * data.cols;


	gint mask_width = gimp_drawable_width(vals->mask_drawable_id);
	gint mask_height = gimp_drawable_height(vals->mask_drawable_id);

	if((data.xmax > mask_width)||(data.ymax > mask_height)) {
#ifdef DEBUG
		g_warning("mask size = width = %d, height = %d",mask_width,mask_height);
#endif
		err = ERR_MASK_DIM;
		ErrorMessage(err);
		ClearMemory(&data);
		return;
	}

	gimp_progress_init ("Inpainting...");
#ifdef DEBUG
	g_warning("before GetImageAndMask");
#endif
	err = GetImageAndMask(image,mask,&data);
#ifdef DEBUG
	g_warning("after GetImageAndMask");
#endif
	data.ordergiven = 0;

	if( err ) {
		ErrorMessage(err);
		ClearMemory(&data);
		return;
	}

	if (vals->stop_path_id != -1) {
		err = CalculateOrderFromPath(vals->stop_path_id,&data);
		if (err) {
			ErrorMessage(err);
			ClearMemory(&data);
			return;
		}
		data.ordergiven = 1;
	}

	if( data.guidance == 1)
		SetKernels(&data);

#ifdef DEBUG
	g_warning("before InpaintImage");
#endif
	InpaintImage(&data);
#ifdef DEBUG
	g_warning("after InpaintImage");
#endif
	//gimp_progress_init ("Inpainting done");

	if(data.inpaint_undefined == 1) {
		g_message("\n\n");
		g_message("Error:\n");
		g_message("Some inpainted image values are undefined !\n");
		g_message("This happens if the order is not well-defined. \n");
		g_message("You can find out the undefined pixels by: > ind = find( isnan(result) ) \n\n\n");
	}

	//CopyMask(&data);
	//CopyParam( &data );
	//CopyDirfield( &data );
	//CopyMImage( &data );
	//CopyTfield(&data);
#ifdef DEBUG
	g_warning("before SetImageAndMask");
#endif
	err = SetImageAndMask(output,mask,&data);
#ifdef DEBUG
	g_warning("after SetImageAndMask");
#endif

	gimp_drawable_detach(image);
	gimp_drawable_detach(mask);
	if (vals->output_drawable_id != vals->image_drawable_id) {
		gimp_drawable_detach(output);
	}
#ifdef DEBUG
	g_warning("before ClearMemory");
#endif
	ClearMemory(&data);
#ifdef DEBUG
	g_warning("after ClearMemory");
#endif
}
