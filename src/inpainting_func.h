/* inpainting_func.h  --- inpaintBCT
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

#ifndef INPAINTING_FUNC_H_
#define INPAINTING_FUNC_H_


#include <math.h>
#include <limits>
struct Data;

#include "Heap.h"



struct Data
{
    // image info
    int rows;
    int cols;
    int xmin,xmax,ymin,ymax;
    int channels;
    int size;
    double *Image;
    double *MImage;

    // data domain info
    double *Domain;
    double *MDomain;

    // time info
    hItem **heap;
    hItem *Tfield;
    double *ordered_points;
    int *inpaint_index;
    int nof_points2inpaint;

    // parameters
    int radius;
    double epsilon;
    double kappa;
    double sigma;
    double rho;
    double thresh;
    double delta_quant4;
    double *convex;

    // smoothing kernels and buffer
    int lenSK1;
    int lenSK2;
    double *SKernel1;
    double *SKernel2;
    double *Shelp;

    // inpaint buffer
    double *Ihelp;

    // flags
    int ordergiven;
    int guidance;
    int inpaint_undefined;

    // extension
    double *GivenGuidanceT;
};

void InpaintImage(Data *data);
void SmoothImage(Data *data);
void OrderByDistance(Data *data);
void InitTfieldAndHeap(Data *data, Heap *H);
void TfieldDefaultInitialization(Data *data);
int TfieldAdaptInitializationToImage(Data *data);
double solve(Data *data, int i,int j);
void InpaintByOrder(Data *data);
void SmoothUpdate(Data *data,int xi,int xj);
void inpaintPoint(Data *data,int i,int j);
void Guidance(Data *data, int xi, int xj, double *G);
void ModStructureTensor(Data *data, int xi,int xj, double *st);
double euclidean_norm(double *v);


#endif /* INPAINTING_FUNC_H_ */
