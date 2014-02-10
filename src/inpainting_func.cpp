/* inpainting_func.cpp  --- inpaintBCT 
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

#include "inpainting_func.h"

#include <cstdlib>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>


#define Inf               std::numeric_limits<double>::infinity()
#define min(a,b)          ((a)<(b)?(a):(b))
#define max(a,b)          ((a)>(b)?(a):(b))
#define round(a)	      (int)((a) + 0.5)
#define sign(a)		      ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))


void InpaintImage(Data *data)
{    
    if(data->guidance == 1)
        SmoothImage(data);
    if( data->ordergiven == 0 )
        OrderByDistance(data);

	InpaintByOrder(data);
    /* debug: for data->thresh > 0
    {
        int i;
        for(i=0 ; i < data->size ; i++ )
            if(data->Tfield[i].T == 0)
                data->Image[i] = 255;
    }
    //*/
}


// Smoothing
void SmoothImage(Data *data)
{
	int i,j,c,ri;
	int p,h,ph;
	int s;
    int size;
    int index;
    
    if(data->SKernel1 == NULL) // i.e. sigma == 0
        return;
    
	s = (data->lenSK1 - 1)/2;
	
	for( c=0 ; c <= data->channels ; c++)
	{
		for( i=0 ; i < data->rows ; i++)
		{
			for( j= -s ; j < data->cols + s ; j++)
			{
				p = (j+s) % data->lenSK1;
				data->Shelp[p] = 0;
				
				// colum sums
				if( (0 <= j) && ( j < data->cols) )
				{
					// colum sums
					for( h = 0 ; h < data->lenSK1 ; h++)
					{
						ri = i-s+h;
						if( (ri < 0) || (ri >= data->rows) )
							continue;
						
                        index = j * data->rows + ri;
                        
						if( c == data->channels )
                            data->Shelp[p] = data->Shelp[p] + data->SKernel1[h] * data->Domain[index];
						else
							data->Shelp[p] = data->Shelp[p] + data->SKernel1[h] * data->Image[index + c * data->size];
					}
				}
				
				if( j >= s )
				{
                    index = (j-s) * data->rows + i;
                    
					if( c == data->channels )
						data->MDomain[index]  = 0;
					else
						data->MImage[index + c * data->size] = 0;
					
					
					for( h = 0 ; h < data->lenSK1 ; h++)
					{
						ph = (p+1+h) % data->lenSK1;
						
						if( c == data->channels )
							data->MDomain[index] = data->MDomain[index] + data->SKernel1[h] * data->Shelp[ph];
						else
							data->MImage[index + c * data->size] = data->MImage[index + c * data->size] + data->SKernel1[h] * data->Shelp[ph];
					}
				}
			}
		}
	}
}
// end smoothing

// procs for inpainting
void InpaintByOrder(Data *data)
{
	int k,kk,kold;
    int i;
    int j;
    int stop;
    int index;
    double Told,Tact;
    
    stop = 3 * data->nof_points2inpaint;
    
    kold = 0;
    Told = data->ordered_points[2];
    
	for( k=0 ; k < stop ; k=k+3 )
	{
		if (k%500==0) gimp_progress_update(0.3 + 0.6*(gdouble)k/(gdouble)(stop));
        Tact = data->ordered_points[k+2];
        
        if( Tact > Told ) // update
        {
            for( kk=kold; kk < k ; kk=kk+3)
            {
                i = (int) (data->ordered_points[kk]);
                j = (int) (data->ordered_points[kk+1]);
                index = j * data->rows + i;
                
                data->Tfield[index].flag = KNOWN;
                data->Domain[index] = 1;
                SmoothUpdate(data,i,j);
            }
            kold = k;
            Told = Tact;
        }
		i = (int) (data->ordered_points[k]);
        j = (int) (data->ordered_points[k+1]);

		inpaintPoint(data,i,j);
	}
}

void inpaintPoint(Data *data,int xi,int xj)
{
    int indexx;
    int indexy;
	int yi,yj;
    int c;
	
    double G[3];
	double z;
	double vi,vj;
	double r;
    double w;
    double W = 0;
    double Wk = 0;
    
    indexx = xj * data->rows + xi;
	
	//init Ihelp
	for( c=0 ; c < data->channels ; c++ )
		data->Ihelp[c] = 0;

    if( data->guidance == 1 )
        Guidance(data,xi,xj,G);
    
    if( data->guidance == 2 )
    {
        G[0] = data->GivenGuidanceT[indexx];
        G[1] = data->GivenGuidanceT[indexx + data->size];
        G[2] = data->GivenGuidanceT[indexx + 2*data->size];
    }
    
	for( yi = max(xi - data->radius,0); (yi <= xi + data->radius) && ( yi < data->rows); yi++)
	{
		for(yj = max(xj - data->radius,0); (yj <= xj + data->radius) && (yj < data->cols); yj++)
		{

            indexy = yj * data->rows + yi;
            
			if( data->Tfield[indexy].flag != KNOWN)
				continue;

			if( data->Tfield[indexy].T == data->Tfield[indexx].T )
				continue;
							
			vi = yi-xi;
			vj = yj-xj;
			r = sqrt( vi*vi + vj*vj );
			
			if(r > data->radius)
				continue;

			// compute weight
            if(data->guidance != 0)
            {
                z = (data->kappa)/(data->epsilon);
                z = z * z;
                z = z * (G[0]*vi*vi + 2*G[1]*vi*vj + G[2]*vj*vj);
                w  = exp(-z * 0.5 )/r;
                
            }
            else
            {
                w = 1/r;
            }
            
            Wk = Wk + w;
            w = 1 + (1.844674407370955e+19 * w); // insurance
			W = W + w;
            
			// average image values
			for( c=0 ; c < data->channels ; c++ )
				data->Ihelp[c] = data->Ihelp[c] + w * data->Image[indexy + c * data->size];
		}
	}
	
    
	if( W == 0 ) 
	{
		// Wk == 0 :may happen if kappa is too large
        // W == 0 : happens if order not well defined or epsilon is too small
        data->inpaint_undefined = 1;
        
        // debug
		// mexPrintf(" Wk is %lf , W is %lf , at %d %d \n",Wk,W,xi,xj );
	}
    
	for( c=0 ; c < data->channels ; c++ )
    {
		data->Image[indexx + c * data->size ] = data->Ihelp[c]/W;
        
        // debug
        // if( isnan( data->Image[indexx + c * data->size] ) )
        //    mexPrintf(" pixel value is %lf at %d %d \n",data->Image[indexx + c * data->size],xi,xj );
    }
    
    data->Domain[indexx] = 1; // inpainting domain shrinks by one pixel
}

void Guidance(Data *data, int xi, int xj, double *G)
{
	double ST[3];
    double diff;
    double coh_meas;
    double coh_meas_sqrt;
    double confidence;
	
   
    ModStructureTensor(data,xi,xj,ST);

    diff = ST[0] - ST[2];
    coh_meas = diff * diff + 4*ST[1]*ST[1];

    coh_meas_sqrt = sqrt(coh_meas);

    if( coh_meas == 0)
        confidence = 0;
    else
        confidence = exp( -(data->delta_quant4) / coh_meas ) / coh_meas_sqrt;
        

    G[0] = 0.5 * confidence * (diff + coh_meas_sqrt);
    G[1] = confidence * ST[1];
    G[2] = 0.5 * confidence * (-diff + coh_meas_sqrt);
	
}

void ModStructureTensor(Data *data, int xi,int xj, double *ST)
{
    int indexx;
    int indexr;
    int indexrc;
	int ri,rj;
	int i,j,c;
	int r;
	double w,wh;
	double vs[3];
	double vsh[3];
	double u0,u1;
	double dx,dy;

	ST[0] = 0;
	ST[1] = 0;
	ST[2] = 0;
	
    indexx = xj * data->rows + xi;
	r = (data->lenSK2-1)/2;

    
	for( c=0 ; c < data->channels ; c++) // for each color channel
	{
		// structure tensor
		vs[0] = 0;
		vs[1] = 0;
		vs[2] = 0;
		w = 0;
		for(i = 0; i < data->lenSK2 ; i++)
		{
			ri = xi+r-i;
			if( (ri < 0) || (ri >= data->rows) )
				continue;

			vsh[0] = 0;
			vsh[1] = 0;
			vsh[2] = 0;
			wh = 0;
			for(j = 0; j < data->lenSK2 ; j++)
			{
				rj = xj+r-j;
				if( (rj < 0) || (rj >= data->cols) )
					continue;
				
                indexr = rj * data->rows + ri;
                
				if(data->Tfield[indexr].T >= data->Tfield[indexx].T)
					continue;

                indexrc = indexr + c * data->size;
                
				// values
				if( (ri==0) || (data->MDomain[indexr - 1] == 0) )
					u0 = data->MImage[indexrc]/data->MDomain[indexr];
				else
					u0 = data->MImage[indexrc - 1]/data->MDomain[indexr - 1];

				if( (ri== data->rows-1) || (data->MDomain[indexr + 1] == 0) )
					u1 = data->MImage[indexrc]/data->MDomain[indexr];
				else
					u1 = data->MImage[indexrc + 1]/data->MDomain[indexr + 1];
				
				dx = (u1 - u0)/2;

				if( (rj==0) || (data->MDomain[indexr - data->rows] == 0) )
					u0 = data->MImage[indexrc]/data->MDomain[indexr];
				else
					u0 = data->MImage[indexrc - data->rows]/data->MDomain[indexr - data->rows];

				if( (rj== data->cols-1) || (data->MDomain[indexr + data->rows] == 0) )
					u1 = data->MImage[indexrc]/data->MDomain[indexr];
				else
					u1 = data->MImage[indexrc + data->rows]/data->MDomain[indexr + data->rows];

				dy = (u1 - u0)/2;
				
                
				vsh[0] = vsh[0] + data->SKernel2[j] * dx * dx;
				vsh[1] = vsh[1] + data->SKernel2[j] * dx * dy;
				vsh[2] = vsh[2] + data->SKernel2[j] * dy * dy;
				wh = wh + data->SKernel2[j];
			}
			vs[0] = vs[0] + data->SKernel2[i] * vsh[0];
			vs[1] = vs[1] + data->SKernel2[i] * vsh[1];
			vs[2] = vs[2] + data->SKernel2[i] * vsh[2];
			w = w + data->SKernel2[i] * wh;
		}
		
		if( data->convex == NULL )
		{
			ST[0] = ST[0] + vs[0]/w;
			ST[1] = ST[1] + vs[1]/w;
			ST[2] = ST[2] + vs[2]/w;
		}
        else
		{
			ST[0] = ST[0] + data->convex[c] * vs[0]/w;
			ST[1] = ST[1] + data->convex[c] * vs[1]/w;
			ST[2] = ST[2] + data->convex[c] * vs[2]/w;
		}
		
	}

	if(data->convex == NULL)
	{
        double W = 1.0 / (data->channels);  
		ST[0] = ST[0] * W;
		ST[1] = ST[1] * W; 
		ST[2] = ST[2] * W; 
	}
}

void SmoothUpdate(Data *data,int xi,int xj)
{
	int yi,yj;
	int i,j,c;
	int s;
    int indexx;
    int indexy;
    
    indexx = xj * data->rows + xi;
    
    if( data->SKernel1 == NULL ) // i.e. sigma == 0
    {
        for(c = 0; c < data->channels ; c++)
			data->MImage[indexx + c * data->size] = data->Image[indexx + c * data->size];  
        
        data->MDomain[indexx] = 1;
        return;
    }
    
	s = (data->lenSK1-1)/2;

	for( yi = xi-s ; yi <= xi+s ; yi++)
		for( yj = xj-s ; yj <= xj+s ; yj++)
		{
			if( (yi >= 0) && (yi < data->rows) && (yj >= 0) && (yj < data->cols) )
			{
				i = xi-yi+s;
				j = xj-yj+s;

                indexy = yj * data->rows + yi;
                
				for(c = 0; c < data->channels ; c++)
                    data->MImage[indexy + c * data->size] += (data->SKernel1[i] * data->SKernel1[j] * data->Image[indexx + c * data->size]);
				
				data->MDomain[indexy] += (data->SKernel1[i] * data->SKernel1[j]);
			}
		}
}
// end procs for inpainting

// procs to compute the order
void OrderByDistance(Data *data)
{
	hItem actual;
	hItem *nbh[4];
	Heap NarrowBand(data);
    int index;
	int i = 0;
    int k,p;
	
    InitTfieldAndHeap(data, &NarrowBand);

    p = 0;
	while(!NarrowBand.isempty())
	{
		if (p++%300==0) gimp_progress_update(0.1+0.2*(gdouble)p/(gdouble)(data->nof_points2inpaint));
		actual = NarrowBand.extract();
        
        index = actual.j * data->rows + actual.i;
       
        data->ordered_points[i]   = actual.i;
        data->ordered_points[i+1] = actual.j;
        data->ordered_points[i+2] = actual.T;
        i = i+3;

		data->Tfield[index].flag = TO_INPAINT;

		if(actual.i == 0) // top
			nbh[0] = NULL;
		else
			nbh[0] = &(data->Tfield[index - 1]);
        
		if(actual.i == data->rows - 1) // bottom
			nbh[1] = NULL;
		else
			nbh[1] = &(data->Tfield[index + 1]);
        
		if(actual.j == 0) // left
			nbh[2] = NULL;
		else
			nbh[2] = &(data->Tfield[index - data->rows]);
        
		if(actual.j == data->cols - 1) // right
			nbh[3] = NULL;
		else
			nbh[3] = &(data->Tfield[index + data->rows]);

		for(k=0 ; k<4; k++)
		{
			if(nbh[k] != NULL)
			{
				if(nbh[k]->flag == INSIDE)
					nbh[k]->flag = BAND;
				if(nbh[k]->flag == BAND)
				{
					hItem temp = *nbh[k];
					temp.T = solve(data, temp.i, temp.j);
					NarrowBand.insert(temp);
				}
			}
		}
	}
}

void InitTfieldAndHeap(Data *data, Heap *H)
{
	int i,j;
    int index;
    hItem item;
    int err =0;
    
    // Initialization of boundary points
    // step 1 : for all bpoints do set flag=BAND, T=0
    // step 2 : for all bpoints do if dot(c,N)*coh < thresh then set flag=BAND, T=0 else set flag=BAND, T=d where d=diam
    // step 3 : Heap Init if flag=BAND and T=0 set flag=TO_INPAINT

    
    // Default Initialization
	TfieldDefaultInitialization(data);
    
    // Change Initialization depending on the image
    if( data->thresh > 0 )
        err = TfieldAdaptInitializationToImage(data);
    
    if( err )
    {
        TfieldDefaultInitialization(data);
        
        g_message("Threshold aplha too large. Falling back to euclidean distance. \n");
    }
        
    
    // Heap Initialization
    for(i = 0; i < data->rows; i++)
	{
		for(j = 0; j < data->cols; j++)
		{
            index = j * data->rows + i;
            
			item = data->Tfield[index];
			H->insert(item);

			// first Boundary is known
			if( (data->Tfield[index].flag == BAND) && (data->Tfield[index].T == 0) )
				data->Tfield[index].flag = TO_INPAINT;
		}
	}
}

void TfieldDefaultInitialization(Data *data)
{
    int i,j;
    int index;
    
    for(i = 0; i < data->rows ; i++)
	{
		for(j = 0; j < data->cols ; j++)
		{
            index = j * data->rows + i;
            
			data->Tfield[index].i = i;
			data->Tfield[index].j = j;
			data->Tfield[index].hpos = -1;

			if(data->Domain[index] == 0)
			{
				data->Tfield[index].flag = BAND;
				data->Tfield[index].T = 0;
			
				if(    ((i==0) || (data->Domain[ index-1 ] == 0)) 
					&& ((i==data->rows-1) || (data->Domain[ index+1 ] == 0)) 
					&& ((j==0) || (data->Domain[ index - data->rows ] == 0)) 
					&& ((j==data->cols-1) || (data->Domain[ index + data->rows ] == 0))  )
				{
					data->Tfield[index].flag = INSIDE;
					data->Tfield[index].T = Inf;
				}

			}
			else
			{
				data->Tfield[index].flag = KNOWN;
				data->Tfield[index].T = -1; 
			}

		}
	}
}


int TfieldAdaptInitializationToImage(Data *data)
{
	int i,j;
    int index;
    double normd;
    double normaldir[2];
    double Dx;
    double ST[3];
    double diff;
    double coh_meas;
    double coh_meas_sqrt;
    double confidence;
	double G[3];
    int countInitialPoints = 0;
    int err = 0;
    
	for(i = 0; i < data->rows ; i++)
	{
		for(j = 0; j < data->cols ; j++)
		{
            index = j * data->rows + i;
            
			if(data->Tfield[index].flag == BAND)
			{               
                // boundary normal
                if( i==0 )
                    normaldir[0] = data->MDomain[index+1]-data->MDomain[index];
                else if( i==data->rows-1 )
                    normaldir[0] = data->MDomain[index]-data->MDomain[index-1];
                else
                {
                    normaldir[0] = data->MDomain[index+1]-data->MDomain[index-1];
                    normaldir[0] = normaldir[0] * 0.5;
                }
                
                if( j==0 )
                    normaldir[1] = data->MDomain[index + data->rows]-data->MDomain[index];
                else if( j==data->cols-1 )
                    normaldir[1] = data->MDomain[index]-data->MDomain[index - data->rows];
                else
                {
                    normaldir[1] = data->MDomain[index + data->rows]-data->MDomain[index - data->rows];
                    normaldir[1] = normaldir[1] * 0.5;
                }
                
                normd = euclidean_norm(normaldir);
                normd = normd*normd + 1e-15;
                
                // guidance
                Guidance(data,i,j,G);
                
                // Dx = Nperp' * G Nperp
                Dx = normaldir[1]*normaldir[1]*G[0] - 2*normaldir[0]*normaldir[1]*G[1] + normaldir[0]*normaldir[0]*G[2];
                Dx = Dx/normd;
                
                
				if( Dx <= data->thresh  )  
				{
					data->Tfield[index].flag = INSIDE; 
					data->Tfield[index].T = Inf;
				}
                else
                {
                    data->Tfield[index].flag = BAND;
                    data->Tfield[index].T = 0;
                    countInitialPoints++;
                }
                
                /*
                if( (i == 0) || (i==100) )
                    mexPrintf(" n: %lf %lf Dx: %lf ST: %lf %lf %lf coh: %lf\n",normaldir[0]/normd,normaldir[1]/normd,Dx,ST[0],ST[1],ST[2],maxeig-mineig);
                */
            }
		}
	}
    
    if( countInitialPoints == 0 )
    {
        err = 1;
    }
    
    return err;
}

double solve(Data *data, int i,int j)
{
	double u[4];
	double ux,uy;
	double u0,u1;
	double diff;
	double U;
	double F = 1;
    int index;
    
    index = j * data->rows + i;
    
	if( (i == 0) || (data->Domain[index-1] == 1) )
		u[0] = Inf;
	else
		u[0] = (data->Tfield[index-1].T);

	if( (i == data->rows - 1) || (data->Domain[index+1] == 1) )
		u[1] = Inf;
	else
		u[1] = (data->Tfield[index+1].T);

	if( (j == 0) || (data->Domain[index - data->rows] == 1) )
		u[2] = Inf;
	else
		u[2] = (data->Tfield[index - data->rows].T);

	if( (j == data->cols - 1) || (data->Domain[index + data->rows] == 1) )
		u[3] = Inf;
	else
		u[3] = (data->Tfield[index + data->rows].T);
    

	ux = min(u[0],u[1]);
	uy = min(u[2],u[3]);

	if(( ux == Inf ) && ( uy == Inf ))
		return Inf;

	u0 = min(ux,uy);
	u1 = max(ux,uy);

	diff = u1-u0;

	if (diff >= 1/F) 
		U = u0 + 1/F;
	else 
		U = (ux+uy+sqrt(2/(F*F) - diff*diff))/2;

	return U;
}
// end procs for order




double euclidean_norm(double *v)
{
	double x1;
	double x2;
	double r;
	double n;

	x1 = fabs(v[0]);
	x2 = fabs(v[1]);

	if( x1 > x2 )
	{
		r = x2/x1;
		n = x1 * sqrt(1 + r*r);
	}
	else if( x1 < x2)
	{
		r = x1/x2;
		n = x2 * sqrt(1 + r*r);
	}
	else
		n = x1 * sqrt(2.0);

	return n;
}

