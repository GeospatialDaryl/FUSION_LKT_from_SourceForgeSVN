// DTM3D class definition
//
#include "plansdtm.h"

typedef struct {
	float x;
	float y;
	float z;
} VECTOR3f;

typedef struct {
	COLORREF	Ncolor;					// color of normal lines
	COLORREF	Bcolor;					// color of bold lines
	short	normal;						// interval for normal contour lines 
	short	bold;						// interval for bold contour lines 
	short	column_smooth;				// smoothness factor for normal contour lines 
	short	point_smooth;				// smoothness factor for bold contour lines
	double	offset;						// vertical offset of contours
	double	vertical_exaggeration;		// vertical exaggeration
	BOOL	DrawZeroLine;				// flag to control presence of 0 elevation line
	DBLCOORD	lower_left;				// coordinate of lower left corner of map area 
	DBLCOORD	upper_right;			// coordinate of upper right corner of map area 
} CONTOUR_MAP_3D;

	typedef struct {
		int CellIndex;
		double DistanceValue;
	} CELLDISTANCE;

//////////////////////////////////////////////////////////////////////////////////////

class DTM3D : public PlansDTM {
public:
	CELLDISTANCE* m_CellDistances;
	double m_MinScalingX;
	double m_MinScalingY;
	double m_MinScalingZ;
	double m_MaxScalingX;
	double m_MaxScalingY;
	double m_MaxScalingZ;
	BOOL m_UseScaling;

public:
	void Empty();
	void SetScalingForRendering(double minx, double miny, double minz, double maxx, double maxy, double maxz);
	void DrawDiskSolidWithTexture(double eminx, double eminy, double emaxx, double emaxy, double ve);
	void DrawDiskBase(double ve);
	void DrawDiskSolid(double ve);
	void DrawDiskWireframe(BOOL apron, double ve);
	void CalculateSmoothNormals();
	void AverageNormals();
	// constructors
	DTM3D();
	DTM3D(LPCSTR filename, BOOL LoadElev = FALSE, BOOL UsePatch = TRUE, BOOL DoNormals = TRUE);

	// destructors
	~DTM3D();

	// rendering code
	void DrawGroundFast(int skip, BOOL apron, double ve = 1.0);
	void DrawGroundBase(double ve = 1.0);
	void DrawGroundSurfaceWithColor(COLORREF* colors, unsigned char** ptcolor, double ve, BOOL UseQuads);
	void DrawGroundSurface(double ve=1.0, BOOL UseQuads = FALSE);
	void DrawGroundSurfaceSorted(double ve=1.0, BOOL UseTexture = FALSE, BOOL UseQuads = FALSE, double eminx = 0.0, double eminy = 0.0, double emaxx = 0.0, double emaxy = 0.0);
	void DrawGroundSurfaceSorted2(double ve=1.0, BOOL UseQuads = FALSE);
	void DrawGroundSurfaceFast(int skip, double ve = 1.0, BOOL UseQuads = FALSE);
	void DrawGroundSurfaceWithTexture(double eminx, double eminy, double emaxx, double emaxy, double ve = 1.0, BOOL UseQuads = FALSE, BOOL DrawApron = FALSE);
	void DrawGroundSurfaceWithTextureSorted(double eminx, double eminy, double emaxx, double emaxy, double ve = 1.0, BOOL UseQuads = FALSE);
	void DrawGroundSurfaceWithTiledTexture(double eminx, double eminy, double emaxx, double emaxy, double ve = 1.0, BOOL UseQuads = FALSE);
	void DrawGroundSurfaceWithTextureFast(double eminx, double eminy, double emaxx, double emaxy, int skip, double ve, BOOL UseQuads = FALSE);
	BOOL Draw3DContours(CONTOUR_MAP_3D *ctr);

	void DrawGroundSurfaceUsingEvaluator(double ve, int skip);

	// calculate normal
	void CalculateNormals(void);

	// set normal
	void SetNormalVector(int r, int c);

private:
	BOOL CalculateCellDistances();
	int GetFarthestCorner();
	void CalculateCornerDistances();
	void CalculateNormal(VECTOR3f* p1, VECTOR3f* p2, VECTOR3f* p3, VECTOR3f* n);
	VECTOR3f**	PointNormals;
	BOOL		HaveNormals;
	double		m_CornerDistance[4];	// 0-ll, 1-lr, 2-ur, 3-ul
	double m_ScalingXRange;
	double m_ScalingYRange;
	double m_ScalingZRange;
};
