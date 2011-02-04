// DTM2D class definition
//
#if !defined(DTM2D_Class)
#define DTM2D_Class

#include "plansdtm.h"

typedef struct {
	CDC*	pDC;						// device context for drawing
	CPen*	cpNormalPen;				// pen for normal lines
	CPen*	cpBoldPen;					// pen for bold lines
	COLORREF	ncolor;					// color of normal lines
	COLORREF	bcolor;					// color of bold lines
	short	normal;						// interval for normal contour lines 
	short	bold;						// interval for bold contour lines 
	short	column_smooth;				// smoothness factor for normal contour lines 
	short	point_smooth;				// smoothness factor for bold contour lines 
	BOOL	DrawZeroLine;				// flag to control presence of 0 elevation line
	BOOL	DrawNeatLine;				// flag to control drawing of DTM outline
	DBLCOORD	world_ll;				// world point for ll corner
	DBLCOORD	world_ur;				// world point for ur corner
	DBLCOORD	lower_left;				// coordinate of lower left corner of map area 
	DBLCOORD	upper_right;			// coordinate of upper right corner of map area 
} CONTOUR_MAP;

//////////////////////////////////////////////////////////////////////////////////////

class DTM2D : public PlansDTM {

public:
	// constructors...use constructors from PlansDTM
	DTM2D() : PlansDTM() {}
	DTM2D(LPCSTR filename, BOOL LoadElev = FALSE, BOOL UsePatch = TRUE) : PlansDTM(filename, LoadElev, UsePatch) {}

	// destructors...compiler will call ~PlansDTM() for us
	~DTM2D() {}

	// contour drawing
	BOOL DrawContours(CONTOUR_MAP *ctr);

private:

};

#endif
