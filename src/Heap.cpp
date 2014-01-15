/* heap.cpp  --- inpaintBCT
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

#include "Heap.h"
#include <cstdlib>


Heap::Heap(Data *data)
{
	size = -1;
	heap = data->heap;
    pdata = data;
}

Heap::~Heap()
{
	size = -1;
	heap = NULL;
    pdata = NULL;
}

void Heap::insert(hItem item)
{
    int index;

    index = item.j * pdata->rows + item.i;

	if(item.flag == BAND)
	{
		if(item.hpos == -1) // not yet in heap
		{
			// T update and heap insertion
			pdata->Tfield[index].T = item.T;
			size = size + 1;
			pdata->Tfield[index].hpos = size;
			heap[size] = &(pdata->Tfield[index]);
			upHeap(size);

		}
		else // already in heap
		{
			// only T update
			double oldT = heap[item.hpos]->T;
			heap[item.hpos]->T = item.T;

			if(oldT > item.T)
				upHeap(item.hpos);
			else
				downHeap(item.hpos);
		}
	}
}

hItem Heap::extract()
{
	hItem ret;

	//heap[0]->flag = KNOWN;
	ret = *heap[0];
	
	heap[0] = heap[size];
	heap[0]->hpos = 0;
	size = size - 1;
	downHeap(0);

	return ret;
}

void Heap::upHeap(int pos)
{
	int help = pos;
	hItem *h;

	if( pos == 0) // head of heap
		return;

	while( (help != 0) && (heap[(help-1)/2]->T > heap[pos]->T))
	{
		help = (help-1)/2;
	
		if(help != pos)
		{
			// swap
			h = heap[help];
			heap[help] = heap[pos];
			heap[help]->hpos = help;
			heap[pos] = h;
			heap[pos]->hpos = pos;

		}

		pos = help;
	}
}

void Heap::downHeap(int pos)
{
	int help = 2*pos+1; // left child
	hItem *h;

	if( help > size) // no children
		return;
	
	// compare left with right child, that is help+1, if it exists
	if( (help +1 <= size) && (heap[help]->T > heap[help+1]->T))
		help = help+1;
	// help is now index with Tmin

	while( (help <= size)  && (heap[help]->T < heap[pos]->T))
	{
		// swap
		h = heap[help];
		heap[help] = heap[pos];
		heap[help]->hpos = help;
		heap[pos] = h;
		heap[pos]->hpos = pos;

		pos = help;
		help = 2*help+1; // left child
		// compare left with right child, that is help+1, if it exists
		if( (help +1 <= size) && (heap[help]->T > heap[help+1]->T))
			help = help+1;
			// help is now index with Tmin
	}
}

int Heap::isempty()
{
	if( size == -1)
		return 1;
	else
		return 0;
}

// for debugging
/*
void Heap::heapPrint()
{
	int i,j;

	mexPrintf("\n");

	for(i=0 ; i<= size; i++)
		mexPrintf("% d. point (%d , %d) value %lf \n",i,(heap[i]->i)+1,(heap[i]->j)+1,heap[i]->T);

	mexPrintf("\n");

	for(i = 0; i <= size; i= 2*i+1)
	{	
		for( j=i ; (j < 2*i+1) && ( j <= size) ; j++)
			mexPrintf(" %lf ",heap[j]->T);

		mexPrintf("\n");
	}
}
*/
