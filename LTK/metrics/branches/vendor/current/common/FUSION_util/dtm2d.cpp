// PlansDTM with 2d contouring class 

#include <stdafx.h>
#include <stdio.h>
#include <math.h>

#include "dtm2d.h"
#include "wcs.h"

// data for contouring code...

// edge crossing map array...[0] = number of times the contour crosses a
// cell edge.  [1] = pointer to first side in crossing side list */
int em[16] [2] = {
	{0, '\0'},
	{5, 0},
	{5, 5},
	{4, 10},
	{5, 14},
	{5, 19},
	{4, 24},
	{2, 28},
	{5, 30},
	{4, 35},
	{5, 39},
	{2, 44},
	{4, 46},
	{2, 50},
	{2, 52},
	{0, '\0'}
};

// crossing side list...contains the sides or segments crossed by a contour
// for each cell map type.  NOTE: no contour crossings for cell types 0 & 15
int el[54] = {
	3, 4, 5, 6, 2,           // cell type 1 
	0, 5, 6, 7, 3,           // cell type 2 
	0, 5, 6, 2,              // cell type 3 
	1, 6, 7, 4, 0,           // cell type 4 
	0, 3, 8, 1, 2,           // cell type 5 
	1, 6, 7, 3,              // cell type 6 
	1, 2,                    // cell type 7 
	1, 5, 4, 7, 2,           // cell type 8 
	1, 5, 4, 3,              // cell type 9 
	0, 1, 8, 2, 3,           // cell type 10 
	0, 1,                    // cell type 11 
	0, 4, 7, 2,              // cell type 12 
	0, 3,                    // cell type 13 
	2, 3                     // cell type 14 
};

BOOL DTM2D::DrawContours(CONTOUR_MAP *ctr)
{

	/**************************************************************************
	*
	*  function name: draw_contours
	*
	*     Generates a contour map from a gridded DTM.  Does not produce vector
	*     information, only a plot on screen.
	*
	*     Reference:
	*
	*        Chris Johnston 1986
	*        Contour plots of large data sets.
	*        Computer Language.
	*        May 1986.
	*
	*
	*     cell vertex and side numbering
	*
	*        2----------1----------3
	*        | \                 / |
	*        |   \             /   |
	*        |     5         6     |
	*        |       \     /       |
	*        0         \ /         2
	*        |         / \         |
	*        |       /     \       |
	*        |     4         7     |
	*        |   /             \   |
	*        | /                 \ |
	*        1----------3----------0
	*  description:
	*
	*     Given a dtm file name, contour interval information, colors, and
	*     the desired area within the DTM, draw contours() draws the contour
	*     lines for the dtm unit.  Assumes that all screen scaling and
	*     windowing has already been done.
	*
	*  return value:
	*
	*     Returns 0 to indicate success, -1 to indicate error.
	*
	**************************************************************************/

	long i;                          // loop counter 
	long j;                          // loop counter 
	int l;                           // loop counter for contour levels 
	int start_level;                 // starting contour level for a column 
	int end_level;                   // ending contour level for a column 
	int k;                           // loop counter for cell side crossings 
	int flag;                        // flag to control pen status (up or down) 
	int cell_index;                  // index into edge crossing array 
	int exit_key = 0;				 // keystroke...only [Esc] terminates 
	long start_col;                  // starting column to be contoured 
	long stop_col;                   // ending column to be contoured 
	long start_point;                // starting point to be contoured 
	long stop_point;                 // ending point to be contoured 
	long test_point;				 // temporary value for testing end of grid 
	long leftover_col;               // extra columns not contoured on first pass 
	long leftover_point;             // extra points not contoured on first pass 
	float ave_elev;                  // average elevation of a cell 
	float x_mult = 1.0f;             // relative distance along a cell edge in X 
	float y_mult = 1.0f;             // relative distance along a cell edge in Y 
	double column_x;                 // X coordinate of column (left side) 
	double x, y;					 // point on line
	void *e1 = NULL;                // pointer to array of elevation data 
	void *e2 = NULL;                // pointer to array of elevation data 
	void *temp = NULL;              // temporary...used when swapping e1 and e2 
	POINT pt;						 // screen point
	CONTOUR_MAP temp_ctr;			 // CONTOUR_MAP structure used for recursive calls 
	MSG stopmsg;
	WCS wcs;

	if (!Valid)
		return(FALSE);

	// set up scaling using WCS
	wcs.Scale(ctr->pDC, ctr->world_ll.x, ctr->world_ll.y, ctr->world_ur.x, ctr->world_ur.y);
	wcs.IsoAdjust(ctr->pDC);

	// calculate the starting row and column 
	start_col = (long) floor((ctr->lower_left.x - Header.origin_x) / Header.column_spacing);
	stop_col = (long) ceil((ctr->upper_right.x - Header.origin_x) / Header.column_spacing);
	start_point = (long) floor((ctr->lower_left.y - Header.origin_y) / Header.point_spacing);
	stop_point = (long) ceil((ctr->upper_right.y - Header.origin_y) / Header.point_spacing);

	if (start_col < 0)
		start_col = 0;
	if (stop_col > (Header.columns - 1))
		stop_col = Header.columns - 1;
	if (start_point < 0)
		start_point = 0;
	if (stop_point > (Header.points - 1))
		stop_point = Header.points - 1;

	// if we have elevations in memory, work from memory
	if (HaveElevations) {
		e2 = lpsElevData[start_col];
	}
	else {
		int sizes[] = {sizeof(short), sizeof(int), sizeof(float), sizeof(double)};

		// otherwise read from file
		// open the dtm file 
		if (!OpenModelFile())
			return(FALSE);

		// allocate memory for 2 profiles of dtm data 
		e1 = (void *) malloc((size_t) Header.points * sizes[Header.z_bytes]);

		// check for allocation problems 
		if (!e1) {
			CloseModelFile();

			return(FALSE);
		}

		e2 = (void *) malloc((size_t) Header.points * sizes[Header.z_bytes]);

		// check for allocation problems 
		if (!e2) {
			free(e1);
			CloseModelFile();

			return(FALSE);
		}

		// read the first profile into e2...will be swapped to e1 in loop 
		if (LoadDTMProfile(start_col, e2)) {
			free(e1);
			free(e2);
			CloseModelFile();

			return(FALSE);
		}
	}

	// sweep through the cells and draw contours 
	for (i = start_col; i < stop_col; i += ctr->column_smooth) {
		// check for last column 
		if (i + ctr->column_smooth > stop_col)
			break;

		// calculate the x for the column 
		column_x = Header.origin_x + ((double) i * Header.column_spacing);

		// swap pointers to elevation arrays 
		temp = e1;
		e1 = e2;
		e2 = temp;

		// read next profile 
		if (HaveElevations) 
			e2 = lpsElevData[i + ctr->column_smooth];
		else {
			if (LoadDTMProfile(i + ctr->column_smooth, e2)) {
				free(e1);
				free(e2);
				CloseModelFile();

				return(FALSE);
			}
		}

		// set color for normal interval 
		ctr->pDC->SelectObject(ctr->cpNormalPen);

		// calculate test point for end of data 
		test_point = stop_point - ctr->point_smooth;

		for (j = start_point; j < stop_point; j += ctr->point_smooth) {
			// check for last point 
			if (j > test_point)
				break;

			if (!ctr->DrawZeroLine) {
				// check for 1 or more void area markers (-1) and skip cell 
				if (ReadColumnValue(e1, j) < 0 || ReadColumnValue(e2, j) < 0 || ReadColumnValue(e1, j+ ctr->point_smooth) < 0 || ReadColumnValue(e2, j + ctr->point_smooth) < 0)
					continue;
			}

			// determine cell min/max elevations 
			start_level = (int) __min(ReadColumnValue(e1, j), ReadColumnValue(e2, j));
			start_level = (int) __min(start_level, ReadColumnValue(e1, j + ctr->point_smooth));
			start_level = (int) __min(start_level, ReadColumnValue(e2, j + ctr->point_smooth));
			start_level = start_level - (start_level % ctr->normal);

			end_level = (int) __max(ReadColumnValue(e1, j), ReadColumnValue(e2, j));
			end_level = (int) __max(end_level, ReadColumnValue(e1, j + ctr->point_smooth));
			end_level = (int) __max(end_level, ReadColumnValue(e2, j + ctr->point_smooth));

			// check cell for each possible contour level 
			// loop using start_level can start at -1...use loop starting at 0
			// to draw 0.0 contour
			for (l = ctr->DrawZeroLine ? 0 : start_level; l <= end_level; l += ctr->normal) {
				// calculate the cell index 
				cell_index = 0;
				if (ReadColumnValue(e2, j) >= l)
					cell_index = 1;
				if (ReadColumnValue(e1, j) >= l)
					cell_index |= 2;
				if (ReadColumnValue(e1, j + ctr->point_smooth) >= l)
					cell_index |= 4;
				if (ReadColumnValue(e2, j + ctr->point_smooth) >= l)
					cell_index |= 8;

				// if cell_index is 0 or 15, no contours at this level 
				if (cell_index == 0 || cell_index == 15)
					continue;

				// calculate the average elevation for the cell,
				// use it to adjust the cell_index 
				ave_elev = ((float) ReadColumnValue(e2, j) + (float) ReadColumnValue(e1, j) + (float) ReadColumnValue(e1, j + ctr->point_smooth) + (float) ReadColumnValue(e2, j + ctr->point_smooth)) / 4.0f;

				if (ave_elev <= (float) l)
					cell_index = cell_index ^ 15;

				// if working on a bold interval, set color to bold color 
				if ((l % ctr->bold) == 0)
					ctr->pDC->SelectObject(ctr->cpBoldPen);

				// set flag to force move to first crossing point 
				flag = 1;

				// step through the side or segment crossings held in the
				// crossing side list 
				for (k = 0; k < em[cell_index] [0]; k ++) {

					// calculate the cell size multiplier...different logic
					// for each side or segment.  Multiplier ranges from 0 to 1 
					switch (el[em[cell_index] [1] + k]) {
						case 0:
							x_mult = 0.0f;
							if (ReadColumnValue(e1, j) == ReadColumnValue(e1, j + ctr->point_smooth))
								y_mult = 0.5f;
							else
								y_mult = ((float) (l - ReadColumnValue(e1, j))) / ((float) (ReadColumnValue(e1, j + ctr->point_smooth) - ReadColumnValue(e1, j)));
							break;
						case 1:
							if (ReadColumnValue(e1, j + ctr->point_smooth) == ReadColumnValue(e2, j + ctr->point_smooth))
								x_mult = 0.5f;
							else
								x_mult = ((float) (l - ReadColumnValue(e1, j + ctr->point_smooth))) / ((float) (ReadColumnValue(e2, j + ctr->point_smooth) - ReadColumnValue(e1, j + ctr->point_smooth)));
							y_mult = 1.0f;
							break;
						case 2:
							x_mult = 1.0f;
							if (ReadColumnValue(e2, j) == ReadColumnValue(e2, j + ctr->point_smooth))
								y_mult = 0.5f;
							else
								y_mult = 1.0f - (((float) (l - ReadColumnValue(e2, j + ctr->point_smooth))) / ((float) (ReadColumnValue(e2, j) - ReadColumnValue(e2, j + ctr->point_smooth))));
							break;
						case 3:
							if (ReadColumnValue(e1, j) == ReadColumnValue(e2, j))
								x_mult = 0.5f;
							else
								x_mult = 1.0f - (((float) (l - ReadColumnValue(e2, j))) / ((float) (ReadColumnValue(e1, j) - ReadColumnValue(e2, j))));
							y_mult = 0.0f;
							break;
						case 4:
							if (ave_elev == (float) ReadColumnValue(e1, j))
								x_mult = 0.25f;
							else
								x_mult = 0.5f * ((float) (l - ReadColumnValue(e1, j))) / (ave_elev - (float) ReadColumnValue(e1, j));
							y_mult = x_mult;
							break;
						case 5:
							if (ave_elev == (float) ReadColumnValue(e1, j + ctr->point_smooth))
								x_mult = 0.25f;
							else
								x_mult = 0.5f * ((float) (l - ReadColumnValue(e1, j + ctr->point_smooth))) / (ave_elev - (float) ReadColumnValue(e1, j + ctr->point_smooth));
							y_mult = 1.0f - x_mult;
							break;
						case 6:
							if (ave_elev == (float) ReadColumnValue(e2, j + ctr->point_smooth))
								x_mult =  0.75f;
							else
								x_mult = 1.0f - (0.5f * ((float) (l - ReadColumnValue(e2, j + ctr->point_smooth))) / (ave_elev - (float) ReadColumnValue(e2, j + ctr->point_smooth)));
							y_mult = x_mult;
							break;
						case 7:
							if (ave_elev == (float) ReadColumnValue(e2, j))
								x_mult = 0.75f;
							else
								x_mult = 1.0f - (0.5f * ((float) (l - ReadColumnValue(e2, j))) / (ave_elev - (float) ReadColumnValue(e2, j)));
							y_mult = 1.0f - x_mult;
							break;
						case 8:
							flag = 1;
							continue;
					}
					// move or draw to the crossing point depending on flag 
					x = column_x + Header.column_spacing * (double) x_mult * (double) ctr->column_smooth;
					y = Header.origin_y + ((double) j * Header.point_spacing) + Header.point_spacing * (double) y_mult * (double) ctr->point_smooth;

					// scale to window coords
					pt.x = wcs.WX(x);
					pt.y = wcs.WY(y);

					if (flag) {
						ctr->pDC->MoveTo(pt);
						flag = 0;
					}
					else {
						ctr->pDC->LineTo(pt);
						if ((l % ctr->bold) == 0)
							ctr->pDC->SetPixel(pt, ctr->bcolor);
						else
							ctr->pDC->SetPixel(pt, ctr->ncolor);
					}
				}
				// if working on a bold interval, reset color to normal 
				if ((l % ctr->bold) == 0)
					ctr->pDC->SelectObject(ctr->cpNormalPen);

				// if working on 0 contour, jump to actual start level 
				if (l == 0) {
					l = start_level - ctr->normal;
					// check for void area markers...(-1) 
					if (ReadColumnValue(e1, j) < 0 || ReadColumnValue(e2, j) < 0 || ReadColumnValue(e1, j + ctr->point_smooth) < 0 || ReadColumnValue(e2, j + ctr->point_smooth) < 0)
						break;
				}
			}
		}
		// check for an [Esc] key press
		if (!ctr->pDC->IsPrinting()) {
			if (PeekMessage(&stopmsg, ctr->pDC->GetWindow()->m_hWnd, WM_KEYUP, WM_KEYUP, PM_REMOVE)) {
				if (stopmsg.wParam == VK_ESCAPE) {
					exit_key = 27;
					break;
				}
			}
		}
	}
	if (!HaveElevations) {
		// free profile arrays and close dtm file 
		free(e1);
		free(e2);
		fclose(modelfile);
	}

	if (exit_key == 27)
		return(TRUE);

	// check to see if there is an uncontoured strip along right side 
	leftover_col = (stop_col - start_col) % ctr->column_smooth;
	leftover_point = (stop_point - start_point) % ctr->point_smooth;

	temp_ctr = *ctr;
	temp_ctr.column_smooth = (short) leftover_col;
	temp_ctr.lower_left.x = Header.origin_x + (stop_col - leftover_col) * Header.column_spacing + (Header.column_spacing * 0.1);

	if (leftover_col) {
		DrawContours(&temp_ctr);
	}

	temp_ctr.column_smooth = ctr->column_smooth;
	temp_ctr.lower_left.x = ctr->lower_left.x;
	temp_ctr.lower_left.y = Header.origin_y + (stop_point - leftover_point) * Header.point_spacing + (Header.point_spacing * 0.1);
	temp_ctr.point_smooth = (short) leftover_point;

	// check to see if there is an uncontoured strip along the top 
	if (leftover_point) {
		DrawContours(&temp_ctr);
	}

	// draw neat line
	if (ctr->DrawNeatLine) {
		ctr->pDC->SelectObject(ctr->cpNormalPen);
		ctr->pDC->SelectStockObject(NULL_BRUSH);
		ctr->pDC->Rectangle(wcs.WX(OriginX()), wcs.WY(OriginY()), wcs.WX(OriginX() + Width()), wcs.WY(OriginY() + Height()));
	}

	return(TRUE);
}
