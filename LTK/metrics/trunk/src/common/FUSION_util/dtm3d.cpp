// PlansDTM with 3d rendering class 

#include <stdafx.h>
#include <stdio.h>
#include <math.h>

#include "dtm3d.h"

#define SETGLCOLOR(a) glColor3ub(GetRValue((a)), GetGValue((a)), GetBValue((a)))
#define	PI	3.14159265358979
#define	D2R (PI / 180.0)
#define SECTORSTEP	10

#define SCALEDX(x) (((x) - m_MinScalingX) / m_ScalingXRange)
#define SCALEDY(y) (((y) - m_MinScalingY) / m_ScalingYRange)
#define SCALEDZ(z) (((z) - m_MinScalingZ) / m_ScalingZRange)

int CompareCellDistance(const void *a1, const void *a2) 
{
	if (((CELLDISTANCE*) a1)->DistanceValue > ((CELLDISTANCE*) a2)->DistanceValue)
		return(-1);
	else if (((CELLDISTANCE*) a1)->DistanceValue < ((CELLDISTANCE*) a2)->DistanceValue)
		return(1);
	else
		return(0);
}

DTM3D::DTM3D() : PlansDTM()
{
	HaveNormals = FALSE;
	m_CellDistances = NULL;
	
	SetScalingForRendering(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
}

DTM3D::DTM3D(LPCSTR filename, BOOL LoadElev, BOOL UsePatch, BOOL DoNormals) : PlansDTM(filename, LoadElev, UsePatch)
{
	// calculate normals
	HaveNormals = FALSE;
	if (DoNormals)
		CalculateSmoothNormals();

	m_CellDistances = NULL;

	SetScalingForRendering(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
}

DTM3D::~DTM3D()
{
	Empty();
}

void DTM3D::CalculateNormal(VECTOR3f *p1, VECTOR3f *p2, VECTOR3f *p3, VECTOR3f* n)
{
	float magnitude;

	n->x = (p3->y - p2->y) * (p1->z - p2->z) - (p3->z - p2->z) * (p1->y - p2->y);
	n->y = (p3->z - p2->z) * (p1->x - p2->x) - (p3->x - p2->x) * (p1->z - p2->z);
	n->z = (p3->x - p2->x) * (p1->y - p2->y) - (p3->y - p2->y) * (p1->x - p2->x);

	// normalize
	magnitude = (float) sqrt(n->x * n->x + n->y * n->y + n->z * n->z);
	n->x /= magnitude;
	n->y /= magnitude;
	n->z /= magnitude;
}

void DTM3D::CalculateSmoothNormals()
{
	// calculate normal vectors for terrain...uses average of the 4 faces that surround a point
	// results in very smooth lighting
	long i, j;
	float magnitude;
	VECTOR3f p1, p2, p3, p4, p5;
	VECTOR3f n, n1, n2, n3, n4;

	if (!Valid || !HaveElevations)
		return;

	float UnitCorrection = 1.0;
	if (Header.xy_units == 0 && Header.z_units == 1)
		UnitCorrection = 3.28084f;
	else if (Header.xy_units == 1 && Header.z_units == 0)
		UnitCorrection = 0.3048f;

	// get rid of normals
	if (HaveNormals) {
		for (int i = 0; i < Header.columns; i ++)
			delete [] PointNormals[i];

		delete [] PointNormals;
	}

	PointNormals = new VECTOR3f* [Header.columns];
	for (i = 0; i < Header.columns; i ++)
		PointNormals[i] = new VECTOR3f [Header.points];

	// do interior gridpoints
	p1.x = (float) Header.column_spacing;
	p2.x = 0.0;
	p3.x = p1.x;
	p4.x = p1.x + (float) Header.column_spacing;
	p5.x = p1.x;
	for (i = 1; i < Header.columns - 1; i ++) {
		p1.y = (float) Header.point_spacing;
		p2.y = p1.y;
		p3.y = p1.y + (float) Header.point_spacing;
		p4.y = p1.y;
		p5.y = 0.0;
		for (j = 1; j < Header.points - 1; j ++) {
			p1.z = (float) ReadInternalElevationValue(i, j, TRUE) * UnitCorrection;
			p2.z = (float) ReadInternalElevationValue(i - 1, j, TRUE) * UnitCorrection;
			p3.z = (float) ReadInternalElevationValue(i, j + 1, TRUE) * UnitCorrection;
			p4.z = (float) ReadInternalElevationValue(i + 1, j, TRUE) * UnitCorrection;
			p5.z = (float) ReadInternalElevationValue(i, j - 1, TRUE) * UnitCorrection;

			CalculateNormal(&p3, &p1, &p4, &n1);
			CalculateNormal(&p4, &p1, &p5, &n2);
			CalculateNormal(&p5, &p1, &p2, &n3);
			CalculateNormal(&p2, &p1, &p3, &n4);

			// average the normals
			n.x = (n1.x + n2.x + n3.x + n4.x) / 4.0f;
			n.y = (n1.y + n2.y + n3.y + n4.y) / 4.0f;
			n.z = (n1.z + n2.z + n3.z + n4.z) / 4.0f;

			// normalize
			magnitude = (float) sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
			PointNormals[i] [j].x = n.x / magnitude;
			PointNormals[i] [j].y = n.y / magnitude;
			PointNormals[i] [j].z = n.z / magnitude;

			p1.y += (float) Header.point_spacing;
			p2.y += (float) Header.point_spacing;
			p3.y += (float) Header.point_spacing;
			p4.y += (float) Header.point_spacing;
			p5.y += (float) Header.point_spacing;
		}
		p1.x += (float) Header.column_spacing;
		p2.x += (float) Header.column_spacing;
		p3.x += (float) Header.column_spacing;
		p4.x += (float) Header.column_spacing;
		p5.x += (float) Header.column_spacing;
	}

	// do edges
	// left edge
	i = 0;
	p1.x = 0.0;
	p1.y = (float) Header.point_spacing;
	p3.x = 0.0;
	p3.y = (float) (Header.point_spacing * 2.0);
	p4.x = (float) Header.column_spacing;
	p4.y = p1.y;
	p5.x = 0.0;
	p5.y = 0.0;
	for (j = 1; j < (Header.points - 1); j ++) {
		p1.z = (float) ReadInternalElevationValue(i, j, TRUE) * UnitCorrection;
		p3.z = (float) ReadInternalElevationValue(i, j + 1, TRUE) * UnitCorrection;
		p4.z = (float) ReadInternalElevationValue(i + 1, j, TRUE) * UnitCorrection;
		p5.z = (float) ReadInternalElevationValue(i, j - 1, TRUE) * UnitCorrection;

		CalculateNormal(&p3, &p1, &p4, &n1);
		CalculateNormal(&p4, &p1, &p5, &n2);

		// average the normals
		n.x = (n1.x + n2.x) / 2.0f;
		n.y = (n1.y + n2.y) / 2.0f;
		n.z = (n1.z + n2.z) / 2.0f;

		// normalize
		magnitude = (float) sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		PointNormals[i] [j].x = n.x / magnitude;
		PointNormals[i] [j].y = n.y / magnitude;
		PointNormals[i] [j].z = n.z / magnitude;

		p1.y += (float) Header.point_spacing;
		p3.y += (float) Header.point_spacing;
		p4.y += (float) Header.point_spacing;
		p5.y += (float) Header.point_spacing;
	}
	
	// right edge
	i = Header.columns - 1;
	p1.x = (float) Width();
	p1.y = (float) Header.point_spacing;
	p2.x = (float) (Width() - Header.column_spacing);
	p2.y = p1.y;
	p3.x = p1.x;
	p3.y = (float) (Header.point_spacing * 2.0);
	p5.x = p1.x;
	p5.y = 0.0;
	for (j = 1; j < (Header.points - 1); j ++) {
		p1.z = (float) ReadInternalElevationValue(i, j, TRUE) * UnitCorrection;
		p2.z = (float) ReadInternalElevationValue(i - 1, j, TRUE) * UnitCorrection;
		p3.z = (float) ReadInternalElevationValue(i, j + 1, TRUE) * UnitCorrection;
		p5.z = (float) ReadInternalElevationValue(i, j - 1, TRUE) * UnitCorrection;

		CalculateNormal(&p2, &p1, &p3, &n1);
		CalculateNormal(&p5, &p1, &p2, &n2);

		// average the normals
		n.x = (n1.x + n2.x) / 2.0f;
		n.y = (n1.y + n2.y) / 2.0f;
		n.z = (n1.z + n2.z) / 2.0f;

		// normalize
		magnitude = (float) sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		PointNormals[i] [j].x = n.x / magnitude;
		PointNormals[i] [j].y = n.y / magnitude;
		PointNormals[i] [j].z = n.z / magnitude;

		p1.y += (float) Header.point_spacing;
		p2.y += (float) Header.point_spacing;
		p3.y += (float) Header.point_spacing;
		p5.y += (float) Header.point_spacing;
	}
	
	// bottom edge
	j = 0;
	p1.x = (float) Header.column_spacing;
	p1.y = 0.0;
	p2.x = 0.0;
	p2.y = 0.0;
	p3.x = p1.x;
	p3.y = (float) Header.point_spacing;
	p4.x = (float) (Header.column_spacing * 2.0);
	p4.y = p1.y;
	for (i = 1; i < (Header.columns - 1); i ++) {
		p1.z = (float) ReadInternalElevationValue(i, j, TRUE) * UnitCorrection;
		p2.z = (float) ReadInternalElevationValue(i - 1, j, TRUE) * UnitCorrection;
		p3.z = (float) ReadInternalElevationValue(i, j + 1, TRUE) * UnitCorrection;
		p4.z = (float) ReadInternalElevationValue(i + 1, j, TRUE) * UnitCorrection;

		CalculateNormal(&p2, &p1, &p3, &n1);
		CalculateNormal(&p3, &p1, &p4, &n2);

		// average the normals
		n.x = (n1.x + n2.x) / 2.0f;
		n.y = (n1.y + n2.y) / 2.0f;
		n.z = (n1.z + n2.z) / 2.0f;

		// normalize
		magnitude = (float) sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		PointNormals[i] [j].x = n.x / magnitude;
		PointNormals[i] [j].y = n.y / magnitude;
		PointNormals[i] [j].z = n.z / magnitude;

		p1.x += (float) Header.column_spacing;
		p2.x += (float) Header.column_spacing;
		p3.x += (float) Header.column_spacing;
		p4.x += (float) Header.column_spacing;
	}

	// top edge
	j = Header.points - 1;
	p1.x = (float) Header.column_spacing;
	p1.y = (float) Height();
	p2.x = 0.0;
	p2.y = p1.y;
	p4.x = (float) (Header.column_spacing * 2.0);
	p4.y = p1.y;
	p5.x = p1.x;
	p5.y = (float) ( Height() - Header.point_spacing);
	for (i = 1; i < (Header.columns - 1); i ++) {
		p1.z = (float) ReadInternalElevationValue(i, j, TRUE) * UnitCorrection;
		p2.z = (float) ReadInternalElevationValue(i - 1, j, TRUE) * UnitCorrection;
		p4.z = (float) ReadInternalElevationValue(i + 1, j, TRUE) * UnitCorrection;
		p5.z = (float) ReadInternalElevationValue(i, j - 1, TRUE) * UnitCorrection;

		CalculateNormal(&p5, &p1, &p2, &n1);
		CalculateNormal(&p4, &p1, &p5, &n2);

		// average the normals
		n.x = (n1.x + n2.x) / 2.0f;
		n.y = (n1.y + n2.y) / 2.0f;
		n.z = (n1.z + n2.z) / 2.0f;

		// normalize
		magnitude = (float) sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
		PointNormals[i] [j].x = n.x / magnitude;
		PointNormals[i] [j].y = n.y / magnitude;
		PointNormals[i] [j].z = n.z / magnitude;

		p1.x += (float) Header.column_spacing;
		p2.x += (float) Header.column_spacing;
		p4.x += (float) Header.column_spacing;
		p5.x += (float) Header.column_spacing;
	}

	// do corners
	// lower left
	p1.x = 0.0;
	p1.y = (float) Header.point_spacing;
	p2.x = 0.0;
	p2.y = 0.0;
	p3.x = (float) Header.column_spacing;
	p3.y = 0.0;

	p1.z = (float) ReadInternalElevationValue(0, 1, TRUE) * UnitCorrection;
	p2.z = (float) ReadInternalElevationValue(0, 0, TRUE) * UnitCorrection;
	p3.z = (float) ReadInternalElevationValue(1, 0, TRUE) * UnitCorrection;

	CalculateNormal(&p1, &p2, &p3, &PointNormals[0] [0]);

	// upper right
	p1.x = (float) Width();
	p2.x = p1.x;
	p2.y = (float) Height();
	p1.y = p2.y - (float) Header.point_spacing;
	p3.x = p1.x - (float) Header.column_spacing;
	p3.y = p2.y;

	p1.z = (float) ReadInternalElevationValue(Header.columns - 1, Header.points - 2, TRUE) * UnitCorrection;
	p2.z = (float) ReadInternalElevationValue(Header.columns - 1, Header.points - 1, TRUE) * UnitCorrection;
	p3.z = (float) ReadInternalElevationValue(Header.columns - 2, Header.points - 1, TRUE) * UnitCorrection;

	CalculateNormal(&p1, &p2, &p3, &PointNormals[Header.columns - 1] [Header.points - 1]);

	// lower right
	p1.x = (float) (Width() - Header.column_spacing);
	p1.y = 0.0;
	p2.x = (float) Width();
	p2.y = 0.0;
	p3.x = p2.x;
	p3.y = (float) Header.point_spacing;

	p1.z = (float) ReadInternalElevationValue(Header.columns - 2, 0, TRUE) * UnitCorrection;
	p2.z = (float) ReadInternalElevationValue(Header.columns - 1, 0, TRUE) * UnitCorrection;
	p3.z = (float) ReadInternalElevationValue(Header.columns - 1, 1, TRUE) * UnitCorrection;

	CalculateNormal(&p1, &p2, &p3, &PointNormals[Header.columns - 1] [0]);

	// upper left
	p1.x = (float) Header.column_spacing;
	p1.y = (float) Height();
	p2.x = 0.0;
	p2.y = p1.y;
	p3.x = 0.0;
	p3.y = (float) (Height() - Header.point_spacing);

	p1.z = (float) ReadInternalElevationValue(1, Header.points - 1, TRUE) * UnitCorrection;
	p2.z = (float) ReadInternalElevationValue(0, Header.points - 1, TRUE) * UnitCorrection;
	p3.z = (float) ReadInternalElevationValue(0, Header.points - 2, TRUE) * UnitCorrection;

	CalculateNormal(&p1, &p2, &p3, &PointNormals[0] [Header.points - 1]);

	HaveNormals = TRUE;
}

void DTM3D::CalculateNormals(void)
{
	// calculate normal vectors for terrain...uses 1 face (upper right) to compute normal and then averages the normals to
	// smooth things out a bit
	// results in some lighting artifacts...but faster than smooth normals by about 4x
	long i, j;
	VECTOR3f p1, p2, p3;

	if (!Valid || !HaveElevations)
		return;

	float UnitCorrection = 1.0;
	if (Header.xy_units == 0 && Header.z_units == 1)
		UnitCorrection = 3.28084f;
	else if (Header.xy_units == 1 && Header.z_units == 0)
		UnitCorrection = 0.3048f;

	// get rid of normals
	if (HaveNormals) {
		for (int i = 0; i < Header.columns; i ++)
			delete [] PointNormals[i];

		delete [] PointNormals;
	}

	PointNormals = new VECTOR3f* [Header.columns];
	for (i = 0; i < Header.columns; i ++)
		PointNormals[i] = new VECTOR3f [Header.points];

	// step through vertices and compute PointNormals...except last col and row
	for (i = 0; i < (Header.columns - 1); i ++) {
		p1.x = (float) i * (float) Header.column_spacing;
		p2.x = p1.x;
		p3.x = p1.x + (float) Header.column_spacing;

		p1.y = (float) Header.point_spacing;
		p2.y = 0.0;

		for (j = 0; j < (Header.points - 1); j ++) {
			p1.z = (float) ReadInternalElevationValue(i, j + 1, TRUE) * UnitCorrection;

			p2.z = (float) ReadInternalElevationValue(i, j, TRUE) * UnitCorrection;

			p3.y = p2.y;
			p3.z = (float) ReadInternalElevationValue(i + 1, j, TRUE) * UnitCorrection;

			CalculateNormal(&p1, &p2, &p3, &PointNormals[i] [j]);

			p1.y += (float) Header.point_spacing;
			p2.y += (float) Header.point_spacing;
		}
	}

	// fill in top row
	p1.x = (float) Header.column_spacing;
	p1.y = (float) Height();
	p2.x = 0.0;
	p2.y = p1.y;
	p3.x = p2.x;
	p3.y = p1.y - (float) Header.point_spacing;
	for (i = 0; i < (Header.columns - 1); i ++) {
		p1.z = (float) ReadInternalElevationValue(i + 1, Header.points - 1, TRUE) * UnitCorrection;
		p2.z = (float) ReadInternalElevationValue(i, Header.points - 1, TRUE) * UnitCorrection;
		p3.z = (float) ReadInternalElevationValue(i, Header.points - 2, TRUE) * UnitCorrection;

		CalculateNormal(&p1, &p2, &p3, &PointNormals[i] [Header.points - 1]);

		p1.x += (float) Header.column_spacing;
		p2.x += (float) Header.column_spacing;
		p3.x = p2.x;

//		PointNormals[i] [Header.points - 1] = PointNormals[i] [Header.points - 2];
	}

	// fill in right side and upper right corner
	p2.x = (float) Width();
	p2.y = 0.0;
	p1.x = p2.x - (float) Header.column_spacing;
	p1.y = 0.0;
	p3.x = p1.x;
	p3.y = (float) Header.point_spacing;
//	for (j = 0; j < Header.points; j ++) {
	for (j = 0; j < Header.points - 1; j ++) {
		p1.z = (float) ReadInternalElevationValue(Header.columns - 2, j, TRUE) * UnitCorrection;
		p2.z = (float) ReadInternalElevationValue(Header.columns - 1, j, TRUE) * UnitCorrection;
		p3.z = (float) ReadInternalElevationValue(Header.columns - 1, j + 1, TRUE) * UnitCorrection;

		CalculateNormal(&p1, &p2, &p3, &PointNormals[Header.columns - 1] [j]);

		p1.y += (float) Header.point_spacing;
		p3.y += (float) Header.point_spacing;
		p1.y = p2.x;

//		PointNormals[Header.columns - 1] [j] = PointNormals[Header.columns - 2] [j];
	}

	// do upper right corner
	p1.x = (float) Width();
	p2.x = p1.x;
	p2.y = (float) Height();
	p1.y = p2.y - (float) Header.point_spacing;
	p3.x = p1.x - (float) Header.column_spacing;
	p3.y = p2.y;

	p1.z = (float) ReadInternalElevationValue(Header.columns - 1, Header.points - 2, TRUE) * UnitCorrection;
	p2.z = (float) ReadInternalElevationValue(Header.columns - 1, Header.points - 1, TRUE) * UnitCorrection;
	p3.z = (float) ReadInternalElevationValue(Header.columns - 2, Header.points - 1, TRUE) * UnitCorrection;

	CalculateNormal(&p1, &p2, &p3, &PointNormals[Header.columns - 1] [Header.points - 1]);

	HaveNormals = TRUE;

	// average the vertex normals using the 4 surrounding points...but not the center point...point being calculated
	AverageNormals();
}

void DTM3D::DrawGroundFast(int skip, BOOL apron, double ve)
{
	int i;
	int j;
	double px, py;

	if (!Valid || !HaveElevations)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	// draw columns
	for (i = 0; i < Header.columns; i += skip) {
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;

		glBegin(GL_LINE_STRIP);
//		glVertex3d(px, py, MinElev() - 5.0);
		if (apron)
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(0.0));
		for (j = 0; j < Header.points; j += skip) {
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));
			py += Header.point_spacing * (double) skip;
		}
		py -= Header.point_spacing * (double) skip;
//		glVertex3d(px, py, Header.min_z - 5.0);
		if (apron)
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(0.0));
		glEnd();
	}

	// draw rows
	for (j = 0; j < Header.points; j += skip) {
		px = Header.origin_x;
		py = Header.origin_y + ((double) j * Header.point_spacing);

		glBegin(GL_LINE_STRIP);
//		glVertex3d(px, py, Header.min_z - 5.0);
		if (apron)
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(0.0));
		for (i = 0; i < Header.columns; i += skip) {
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			px += Header.column_spacing * (double) skip;
		}
		px -= Header.column_spacing * (double) skip;
//		glVertex3d(px, py, Header.min_z - 5.0);
		if (apron)
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(0.0));
		glEnd();
	}
}

void DTM3D::DrawGroundSurface(double ve, BOOL UseQuads)
{
	int i;
	int j;
	double px, py;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	for (i = 0; i < Header.columns - 1; i ++) {
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points; j ++) {
			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + 1] [j].x,
					   PointNormals[i + 1] [j].y,	
					   PointNormals[i + 1] [j].z);
			
			glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += Header.point_spacing;
		}
		glEnd();
	}
}

//DEL void DTM3D::DrawGroundSurface(double minx, double miny, double minz, double maxx, double maxy, double maxz, double ve, BOOL UseQuads)
//DEL {
//DEL 	int i;
//DEL 	int j;
//DEL 	double px, py;
//DEL 	double xrange = maxx - minx;
//DEL 	double yrange = maxy - miny;
//DEL 	double zrange = maxz - minz;
//DEL 
//DEL 	if (!Valid || !HaveElevations || !HaveNormals)
//DEL 		return;
//DEL 
//DEL 	// adjust for mismatched XY and elevation units
//DEL 	if (Header.xy_units != Header.z_units) {
//DEL 		if (Header.xy_units == 0 && Header.z_units == 1)
//DEL 			ve *= 3.28084;
//DEL 		else if (Header.xy_units == 1 && Header.z_units == 0)
//DEL 			ve *= 0.3048;
//DEL 	}
//DEL 
//DEL 	for (i = 0; i < Header.columns - 1; i ++) {
//DEL 		px = Header.origin_x + ((double) i * Header.column_spacing);
//DEL 		py = Header.origin_y;
//DEL 
//DEL 		if (UseQuads)
//DEL 			glBegin(GL_QUAD_STRIP);
//DEL 		else
//DEL 			glBegin(GL_TRIANGLE_STRIP);
//DEL 
//DEL 		for (j = 0; j < Header.points; j ++) {
//DEL 			glNormal3f(PointNormals[i] [j].x,
//DEL 					   PointNormals[i] [j].y,	
//DEL 					   PointNormals[i] [j].z);
//DEL 			
//DEL 			glVertex3d((px - minx) / xrange, (py - miny) / yrange, (((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z - minz) / zrange);
//DEL 
//DEL 			glNormal3f(PointNormals[i + 1] [j].x,
//DEL 					   PointNormals[i + 1] [j].y,	
//DEL 					   PointNormals[i + 1] [j].z);
//DEL 			
//DEL 			glVertex3d((px + Header.column_spacing - minx) / xrange, (py - miny) / yrange, (((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z - minz) / zrange);
//DEL 
//DEL 			py += Header.point_spacing;
//DEL 		}
//DEL 		glEnd();
//DEL 	}
//DEL }

void DTM3D::DrawGroundSurfaceSorted(double ve, BOOL UseTexture, BOOL UseQuads, double eminx, double eminy, double emaxx, double emaxy)
{
	int i, j;
	int index;
	double px, py;
	double tx, ty;
	double txratio;
	double xextents;
	double yextents;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	if (UseTexture && eminx + eminy + emaxx + emaxy == 0.0)
		UseTexture = FALSE;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	if (CalculateCellDistances()) {
		// sort cells
		qsort(m_CellDistances, (Rows() - 1) * (Columns() - 1), sizeof(CELLDISTANCE), CompareCellDistance);

		// calculate texture stuff
		if (UseTexture) {
			xextents = (emaxx - eminx);
			yextents = (emaxy - eminy);
			txratio = Header.column_spacing / (xextents);
		}

		// step through cells and draw each cell
		for (index = 0; index < (Rows() - 1) * (Columns() - 1); index ++) {
			i = m_CellDistances[index].CellIndex / (Rows() - 1);
			j = m_CellDistances[index].CellIndex % (Rows() - 1);

			px = Header.origin_x + ((double) i * Header.column_spacing);
			py = Header.origin_y + ((double) j * Header.point_spacing);

			if (UseTexture) {
				tx = (px - eminx) / xextents;
				ty = (py - eminy) / yextents;
			}

			if (UseQuads)
				glBegin(GL_QUAD_STRIP);
			else
				glBegin(GL_TRIANGLE_STRIP);

			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			
			if (UseTexture)
				glTexCoord2d(tx, ty);

			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + 1] [j].x,
					   PointNormals[i + 1] [j].y,	
					   PointNormals[i + 1] [j].z);
			
			if (UseTexture)
				glTexCoord2d(tx + txratio, ty);

			glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += Header.point_spacing;
			if (UseTexture)
				ty = (py - eminy) / yextents;

			glNormal3f(PointNormals[i] [j + 1].x,
					   PointNormals[i] [j + 1].y,	
					   PointNormals[i] [j + 1].z);
			
			if (UseTexture)
				glTexCoord2d(tx, ty);

			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j + 1, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + 1] [j + 1].x,
					   PointNormals[i + 1] [j + 1].y,	
					   PointNormals[i + 1] [j + 1].z);
			
			if (UseTexture)
				glTexCoord2d(tx + txratio, ty);

			glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j + 1, TRUE) - Header.min_z) * ve + Header.min_z));

			glEnd();
		}

		// delete cell list
//		delete [] m_CellDistances;
//		m_CellDistances = NULL;
	}
}

void DTM3D::DrawGroundSurfaceSorted2(double ve, BOOL UseQuads)
{
	int i;
	int j;
	int istep;
	int jstep;
	double px, py;

/*	double upvector[3];
	upvector[0] = 0.0;
	upvector[1] = 0.0;
	upvector[2] = 1.0;

	double cosangle = 0.5;
	double dotproduct1;		// upvector . normal
	double dotproduct2;		// upvector . normal
	double ccolor[4];
	glGetDoublev(GL_CURRENT_COLOR, ccolor);
*/
	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	CalculateCornerDistances();
	int farcorner = GetFarthestCorner();

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	switch (farcorner) {
	case 0:
		istep = jstep = 1;
		break;
	case 1:
		istep = -1;
		jstep = 1;
		break;
	case 2:
		istep = jstep = -1;
		break;
	case 3:
		istep = 1;
		jstep = -1;
		break;
	}

	if (istep > 0) {
		for (i = 0; i < Header.columns - 1; i ++) {
			px = Header.origin_x + ((double) i * Header.column_spacing);

			if (UseQuads)
				glBegin(GL_QUAD_STRIP);
			else
				glBegin(GL_TRIANGLE_STRIP);

			if (jstep > 0) {
				py = Header.origin_y;
				for (j = 0; j < Header.points; j ++) {
/*					dotproduct1 = (PointNormals[i] [j].x * upvector[0] + PointNormals[i] [j].y * upvector[1] + PointNormals[i] [j].z * upvector[2]);
					dotproduct2 = (PointNormals[i + 1] [j].x * upvector[0] + PointNormals[i + 1] [j].y * upvector[1] + PointNormals[i + 1] [j].z * upvector[2]);
	
					if (dotproduct1 >= cosangle && dotproduct2 >= cosangle) {
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 1.0);
						glColor4d(1.0, 0.0, 0.0, 1.0);
					}
					else {
						glEnd();
						if (UseQuads)
							glBegin(GL_QUAD_STRIP);
						else
							glBegin(GL_TRIANGLE_STRIP);
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 0.01);
						glColor4d(0.0, 1.0, 0.0, 1.0);
					}
*/
					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
					
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i + 1] [j].x,
							   PointNormals[i + 1] [j].y,	
							   PointNormals[i + 1] [j].z);
					
					glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py += Header.point_spacing;
				}
			}
			else {
				py = Header.origin_y + Height();
				for (j = Header.points - 1; j >= 0; j --) {
/*					dotproduct1 = (PointNormals[i] [j].x * upvector[0] + PointNormals[i] [j].y * upvector[1] + PointNormals[i] [j].z * upvector[2]);
					dotproduct2 = (PointNormals[i + 1] [j].x * upvector[0] + PointNormals[i + 1] [j].y * upvector[1] + PointNormals[i + 1] [j].z * upvector[2]);
	
					if (dotproduct1 >= cosangle && dotproduct2 >= cosangle) {
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 1.0);
						glColor4d(1.0, 0.0, 0.0, 1.0);
					}
					else {
						glEnd();
						if (UseQuads)
							glBegin(GL_QUAD_STRIP);
						else
							glBegin(GL_TRIANGLE_STRIP);
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 0.01);
						glColor4d(0.0, 1.0, 0.0, 1.0);
					}
*/
					glNormal3f(PointNormals[i + 1] [j].x,
							   PointNormals[i + 1] [j].y,	
							   PointNormals[i + 1] [j].z);
					
					glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
					
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py -= Header.point_spacing;
				}
			}
			glEnd();
		}
	}
	else {
		for (i = Header.columns - 1; i > 0; i --) {
			px = Header.origin_x + ((double) i * Header.column_spacing);

			if (UseQuads)
				glBegin(GL_QUAD_STRIP);
			else
				glBegin(GL_TRIANGLE_STRIP);

			if (jstep > 0) {
				py = Header.origin_y;
				for (j = 0; j < Header.points; j ++) {
/*					dotproduct1 = (PointNormals[i] [j].x * upvector[0] + PointNormals[i] [j].y * upvector[1] + PointNormals[i] [j].z * upvector[2]);
					dotproduct2 = (PointNormals[i - 1] [j].x * upvector[0] + PointNormals[i - 1] [j].y * upvector[1] + PointNormals[i - 1] [j].z * upvector[2]);
	
					if (dotproduct1 >= cosangle && dotproduct2 >= cosangle) {
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 1.0);
						glColor4d(1.0, 0.0, 0.0, 1.0);
					}
					else {
						glEnd();
						if (UseQuads)
							glBegin(GL_QUAD_STRIP);
						else
							glBegin(GL_TRIANGLE_STRIP);
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 0.01);
						glColor4d(0.0, 1.0, 0.0, 1.0);
					}
*/
					glNormal3f(PointNormals[i - 1] [j].x,
							   PointNormals[i - 1] [j].y,	
							   PointNormals[i - 1] [j].z);
					
					glVertex3d(SCALEDX(px - Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
					
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py += Header.point_spacing;
				}
			}
			else {
				py = Header.origin_y + Height();
				for (j = Header.points - 1; j >= 0; j --) {
/*					dotproduct1 = (PointNormals[i] [j].x * upvector[0] + PointNormals[i] [j].y * upvector[1] + PointNormals[i] [j].z * upvector[2]);
					dotproduct2 = (PointNormals[i - 1] [j].x * upvector[0] + PointNormals[i - 1] [j].y * upvector[1] + PointNormals[i - 1] [j].z * upvector[2]);
	
					if (dotproduct1 >= cosangle && dotproduct2 >= cosangle) {
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 1.0);
						glColor4d(1.0, 0.0, 0.0, 1.0);
					}
					else {
						glEnd();
						if (UseQuads)
							glBegin(GL_QUAD_STRIP);
						else
							glBegin(GL_TRIANGLE_STRIP);
//						glColor4d(ccolor[0], ccolor[1], ccolor[2], 0.01);
						glColor4d(0.0, 1.0, 0.0, 1.0);
					}
*/
					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
					
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i - 1] [j].x,
							   PointNormals[i - 1] [j].y,	
							   PointNormals[i - 1] [j].z);
					
					glVertex3d(SCALEDX(px - Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py -= Header.point_spacing;
				}
			}
			glEnd();
		}
	}
}

void DTM3D::DrawGroundSurfaceWithColor(COLORREF* colors, unsigned char** ptcolor, double ve, BOOL UseQuads)
{
	int i;
	int j;
	double px, py;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	for (i = 0; i < Header.columns - 1; i ++) {
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points; j ++) {
			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			SETGLCOLOR(colors[ptcolor[i][j]]);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + 1] [j].x,
					   PointNormals[i + 1] [j].y,	
					   PointNormals[i + 1] [j].z);
			
			SETGLCOLOR(colors[ptcolor[i + 1][j]]);
			glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += Header.point_spacing;
		}
		glEnd();
	}
}
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
void DTM3D::DrawGroundSurfaceFast(int skip, double ve, BOOL UseQuads)
{
	if (skip == 1) {
		DrawGroundSurface(ve, UseQuads);
		return;
	}

	int i;
	int j;
	double px, py;
	double xinc, yinc;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	xinc = Header.column_spacing * (double) skip;
	yinc = Header.point_spacing * (double) skip;
	for (i = 0; i < Header.columns - skip; i += skip) {
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points - skip; j += skip) {
			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + skip] [j].x,
					   PointNormals[i + skip] [j].y,	
					   PointNormals[i + skip] [j].z);
			
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + skip, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += yinc;
		}
		// send last point in the column
		if (Header.points % skip) {
			py = Header.origin_y + (double) Header.points * Header.point_spacing;
			glNormal3f(PointNormals[i] [Header.points - 1].x,
					   PointNormals[i] [Header.points - 1].y,	
					   PointNormals[i] [Header.points - 1].z);
			
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + skip] [Header.points - 1].x,
					   PointNormals[i + skip] [Header.points - 1].y,	
					   PointNormals[i + skip] [Header.points - 1].z);
			
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + skip, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));
		}
		glEnd();
	}
	if (Header.columns % skip) {
//		i -= skip;
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points - skip; j += skip) {
			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[Header.columns - 1] [j].x,
					   PointNormals[Header.columns - 1] [j].y,	
					   PointNormals[Header.columns - 1] [j].z);
			
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(Header.columns - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += yinc;
		}
		// send last point in the column
		if (Header.points % skip) {
			py = Header.origin_y + (double) Header.points * Header.point_spacing;
			glNormal3f(PointNormals[i] [Header.points - 1].x,
					   PointNormals[i] [Header.points - 1].y,	
					   PointNormals[i] [Header.points - 1].z);
			
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[Header.columns - 1] [Header.points - 1].x,
					   PointNormals[Header.columns - 1] [Header.points - 1].y,	
					   PointNormals[Header.columns - 1] [Header.points - 1].z);
			
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(Header.columns - 1, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));
		}
		glEnd();
	}
}

void DTM3D::DrawGroundBase(double ve)
{
	int i;
	int j;
	double px, py;
	double BaseElev = -2.0;

	if (!Valid ||!HaveElevations)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	// switch winding order for polygons...apron (base) vertices are sent in the wrong order
//	glFrontFace(GL_CW);

	// bottom side of base
	px = Header.origin_x;
	py = Header.origin_y;
	glBegin(GL_QUAD_STRIP);
	glNormal3f(0, -1, 0);
	for (i = 0; i < Header.columns; i ++) {
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, 0, TRUE) - Header.min_z) * ve + Header.min_z));
//		glVertex3d(px, py, Header.min_z - 5.0);
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(BaseElev));

		px += Header.column_spacing;
	}
	glEnd();

	// 7/24/2002 changed order of vertices on top and right sides to correct problems when backface culling is enabled
	// was sending vertices in wrong order for these edges

	// top side of base
	px = Header.origin_x;
	py = Header.origin_y + (double) (Header.points - 1) * Header.point_spacing;
	glBegin(GL_QUAD_STRIP);
	glNormal3f(0, 1, 0);
	for (i = 0; i < Header.columns; i ++) {
//		glVertex3d(px, py, ((double) ReadInternalElevationValue(i, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z);
//		glVertex3d(px, py, Header.min_z - 5.0);
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(BaseElev));
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));

		px += Header.column_spacing;
	}
	glEnd();

	// right side of base
	px = Header.origin_x;
	py = Header.origin_y;
	glBegin(GL_QUAD_STRIP);
	glNormal3f(-1, 0, 0);
	for (j = 0; j < Header.points; j ++) {
//		glVertex3d(px, py, ((double) ReadInternalElevationValue(0, j, TRUE) - Header.min_z) * ve + Header.min_z);
//		glVertex3d(px, py, Header.min_z - 5.0);
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(BaseElev));
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(0, j, TRUE) - Header.min_z) * ve + Header.min_z));

		py += Header.point_spacing;
	}
	glEnd();

	// left side of base
	px = Header.origin_x + (double) (Header.columns - 1) * Header.column_spacing;
	py = Header.origin_y;
	glBegin(GL_QUAD_STRIP);
	glNormal3f(1, 0, 0);
	for (j = 0; j < Header.points; j ++) {
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(Header.columns - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));
//		glVertex3d(px, py, Header.min_z - 5.0);
		glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(BaseElev));

		py += Header.point_spacing;
	}
	glEnd();

	// base
	glBegin(GL_QUADS);
	glNormal3f(0, 0, -1);
//	glVertex3d(Header.origin_x, Header.origin_y, Header.min_z - 5.0);
//	glVertex3d(Header.origin_x + Width(), Header.origin_y, Header.min_z - 5.0);
//	glVertex3d(Header.origin_x + Width(), Header.origin_y + Height(), Header.min_z - 5.0);
//	glVertex3d(Header.origin_x, Header.origin_y + Height(), Header.min_z - 5.0);
	glVertex3d(SCALEDX(Header.origin_x), SCALEDY(Header.origin_y), SCALEDZ(BaseElev));
	glVertex3d(SCALEDX(Header.origin_x + Width()), SCALEDY(Header.origin_y), SCALEDZ(BaseElev));
	glVertex3d(SCALEDX(Header.origin_x + Width()), SCALEDY(Header.origin_y + Height()), SCALEDZ(BaseElev));
	glVertex3d(SCALEDX(Header.origin_x), SCALEDY(Header.origin_y + Height()), SCALEDZ(BaseElev));
	glEnd();

	// reset winding order for polygons...apron (base) vertices are sent in the wrong order
//	glFrontFace(GL_CCW);
}


// data for contouring code...

// edge crossing map array...[0] = number of times the contour crosses a
// cell edge.  [1] = pointer to first side in crossing side list */
int em3d[16] [2] = {
	{0, 0},
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
	{0, 0}
};

// crossing side list...contains the sides or segments crossed by a contour
// for each cell map type.  NOTE: no contour crossings for cell types 0 & 15
int el3d[54] = {
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

BOOL DTM3D::Draw3DContours(CONTOUR_MAP_3D *ctr)
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
	CONTOUR_MAP_3D temp_ctr;		 // 3D_CONTOUR_MAP structure used for recursive calls 
	double ve = ctr->vertical_exaggeration;
//	MSG stopmsg;

	if (!Valid)
		return(FALSE);

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

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
		if (!LoadDTMProfile(start_col, e2)) {
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
			if (!LoadDTMProfile(i + ctr->column_smooth, e2)) {
				free(e1);
				free(e2);
				CloseModelFile();

				return(FALSE);
			}
		}

		// set color for normal interval
		SETGLCOLOR(ctr->Ncolor);

		// calculate test point for end of data 
		test_point = stop_point - ctr->point_smooth;

		for (j = start_point; j < stop_point; j += ctr->point_smooth) {
			// check for last point 
			if (j > test_point)
				break;

			if (!ctr->DrawZeroLine) {
				// check for 1 or more void area markers (-1) and skip cell 
				if (ReadColumnValue(e1, j) < 0 || ReadColumnValue(e2, j) < 0 || ReadColumnValue(e1, j + ctr->point_smooth) < 0 || ReadColumnValue(e2, j + ctr->point_smooth) < 0)
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
					SETGLCOLOR(ctr->Bcolor);

//if (cell_index == 1 || cell_index == 2 || cell_index == 4 || cell_index == 8)
	//glColor3f(1.0, 0, 0);

				// step through the side or segment crossings held in the
				// crossing side list 
				glBegin(GL_LINE_STRIP);

				// set normal for lower left corner of cell
//				glNormal3f(PointNormals[i] [j].x,
//						   PointNormals[i] [j].y,	
//						   PointNormals[i] [j].z);

				for (k = 0; k < em3d[cell_index] [0]; k ++) {

					// calculate the cell size multiplier...different logic
					// for each side or segment.  Multiplier ranges from 0 to 1 
					switch (el3d[em3d[cell_index] [1] + k]) {
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
							glEnd();
							glBegin(GL_LINE_STRIP);
							// set normal for lower left corner of cell
//							glNormal3f(PointNormals[i] [j].x,
//									   PointNormals[i] [j].y,	
//									   PointNormals[i] [j].z);

							continue;
					}
					// set vertex to crossing point
					x = column_x + Header.column_spacing * (double) x_mult * (double) ctr->column_smooth;
					y = Header.origin_y + ((double) j * Header.point_spacing) + Header.point_spacing * (double) y_mult * (double) ctr->point_smooth;

					glVertex3d(SCALEDX(x), SCALEDY(y), SCALEDZ((((double) l + ctr->offset) - Header.min_z) * ve + Header.min_z));
				}
				glEnd();

				// if working on a bold interval, reset color to normal 
				if ((l % ctr->bold) == 0)
					SETGLCOLOR(ctr->Ncolor);

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
//		if (PeekMessage(&stopmsg, ctr->pDC->GetWindow()->m_hWnd, WM_KEYUP, WM_KEYUP, PM_REMOVE)) {
//			if (stopmsg.wParam == VK_ESCAPE) {
//				exit_key = 27;
//				break;
//			}
//		}
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
	temp_ctr.lower_left.x = Header.origin_x + (stop_col - leftover_col) * Header.column_spacing + 1.0;

	if (leftover_col) {
		Draw3DContours(&temp_ctr);
	}

	temp_ctr.column_smooth = ctr->column_smooth;
	temp_ctr.lower_left.x = ctr->lower_left.x;
	temp_ctr.lower_left.y = Header.origin_y + (stop_point - leftover_point) * Header.point_spacing + 1.0;
	temp_ctr.point_smooth = (short) leftover_point;

	// check to see if there is an uncontoured strip along the top 
	if (leftover_point) {
		Draw3DContours(&temp_ctr);
	}

	return(TRUE);
}

void DTM3D::DrawGroundSurfaceUsingEvaluator(double ve, int skip)
{
	// using the evaluator is a good idea but dtms have too many points (order of surface is 
	// too high for OpenGL)...this implementation, builds an array of control points that
	// will work in OpenGL (uses max order by max order array)...result is a very smoothed
	// surface with no detail

	// texture coords have some problems...surface is black;
	GLfloat texpts[2][2][2] = {{{0.0, 0.0}, {1.0, 0.0}}, {{0.0, 1.0}, {1.0, 1.0}}};

	int mo;
	glGetIntegerv(GL_MAX_EVAL_ORDER, &mo);

	int gridpoints = mo;

	// set up memory for points
	VECTOR3f* pts = new VECTOR3f [mo * mo];

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	// calculate grid point coordinates
	int index;
	float dx = (float) (Width() / (double) (mo - 1));
	float dy = (float) (Height() / (double) (mo - 1));
	float x;
	float y;
	for (int i = 0; i < mo; i ++) {
		index = i * mo;
		x = (float) (Header.origin_x + dx * (double) i);
		y = (float) Header.origin_y;
		for (int j = 0; j < mo; j ++) {
			pts[index].x = x;
			pts[index].y = y;
			pts[index].z = (float) ((InterpolateElev(x, y) - Header.min_z) * ve + Header.min_z);

			index ++;
			y += dy;
		}
	}

	// set up evaluator
	glMap2f(GL_MAP2_VERTEX_3, (GLfloat) Header.origin_x, (GLfloat) (Header.origin_x + Width()), mo * 3, mo, (GLfloat) Header.origin_y, (GLfloat) (Header.origin_y + Height()), 3, mo, &pts[0].x);
	glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &texpts[0][0][0]);

	glEnable(GL_MAP2_TEXTURE_COORD_2);
	glEnable(GL_MAP2_VERTEX_3);
	glEnable(GL_AUTO_NORMAL);

	glMapGrid2f(gridpoints, (GLfloat) Header.origin_x, (GLfloat) (Header.origin_x + Width()), gridpoints, (GLfloat) Header.origin_y, (GLfloat) (Header.origin_y + Height()));
//	glMapGrid2f(Header.columns, Header.origin_x, (GLfloat) (Header.origin_x + ((double) (Header.columns - 1) * Header.column_spacing)), Header.points, Header.origin_y, (GLfloat) (Header.origin_y + ((double) (Header.points - 1) * Header.point_spacing)));

	// draw
//	glTranslated(Header.origin_x, Header.origin_y, 0.0);
	glEvalMesh2(GL_FILL, 0, gridpoints, 0, gridpoints);
//	glEvalMesh2(GL_FILL, 0, Header.columns, 0, Header.points);

	// turn off stuff
	glDisable(GL_MAP2_TEXTURE_COORD_2);
	glDisable(GL_MAP2_VERTEX_3);
	glDisable(GL_AUTO_NORMAL);

	delete [] pts;
}

void DTM3D::DrawGroundSurfaceWithTexture(double eminx, double eminy, double emaxx, double emaxy, double ve, BOOL UseQuads, BOOL DrawApron)
{
	int i;
	int j;
	double px, py;
	double tx, ty;
	double txratio;
	double xextents;
	double yextents;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	xextents = (emaxx - eminx);
	yextents = (emaxy - eminy);
	txratio = Header.column_spacing / (xextents);
	for (i = 0; i < Header.columns - 1; i ++) {
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;
		tx = (px - eminx) / xextents;

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points; j ++) {
			ty = (py - eminy) / yextents;

			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
		
//			if (tx < 0.0 || tx > 1.0 || ty < 0.0 || ty > 1.0)
//				glTexCoord2d(0.0, 0.0);
//			else
				glTexCoord2d(tx, ty);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + 1] [j].x,
					   PointNormals[i + 1] [j].y,	
					   PointNormals[i + 1] [j].z);
			
//			if ((tx + txratio) < 0.0 || (tx + txratio) > 1.0 || ty < 0.0 || ty > 1.0)
//				glTexCoord2d(0.0, 0.0);
//			else
				glTexCoord2d(tx + txratio, ty);
				glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));
			py += Header.point_spacing;
		}
		glEnd();
	}

	if (DrawApron) {
		glColor3ub(0, 0, 0);
		DrawGroundBase(ve);
	}
}

void DTM3D::DrawGroundSurfaceWithTextureSorted(double eminx, double eminy, double emaxx, double emaxy, double ve, BOOL UseQuads)
{
	int i;
	int j;
	int istep;
	int jstep;
	double px, py;
	double tx, ty;
	double txratio;
	double xextents;
	double yextents;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	CalculateCornerDistances();
	int farcorner = GetFarthestCorner();

	switch (farcorner) {
	case 0:
		istep = jstep = 1;
		break;
	case 1:
		istep = -1;
		jstep = 1;
		break;
	case 2:
		istep = jstep = -1;
		break;
	case 3:
		istep = 1;
		jstep = -1;
		break;
	}

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	xextents = (emaxx - eminx);
	yextents = (emaxy - eminy);
	txratio = Header.column_spacing / (xextents);

	if (istep > 0) {
		for (i = 0; i < Header.columns - 1; i ++) {
			px = Header.origin_x + ((double) i * Header.column_spacing);
			tx = (px - eminx) / xextents;

			if (UseQuads)
				glBegin(GL_QUAD_STRIP);
			else
				glBegin(GL_TRIANGLE_STRIP);

			if (jstep > 0) {
				py = Header.origin_y;
				for (j = 0; j < Header.points - 1; j ++) {
					ty = (py - eminy) / yextents;

					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
				
					glTexCoord2d(tx, ty);
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i + 1] [j].x,
							   PointNormals[i + 1] [j].y,	
							   PointNormals[i + 1] [j].z);
					
					glTexCoord2d(tx + txratio, ty);
					glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py += Header.point_spacing;
				}
			}
			else {
				py = Header.origin_y + Height();
				for (j = Header.points - 1; j >= 0; j --) {
					ty = (py - eminy) / yextents;

					glNormal3f(PointNormals[i + 1] [j].x,
							   PointNormals[i + 1] [j].y,	
							   PointNormals[i + 1] [j].z);
					
					glTexCoord2d(tx + txratio, ty);
					glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
				
					glTexCoord2d(tx, ty);
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py -= Header.point_spacing;
				}
			}
			glEnd();
		}
	}
	else {
		for (i = Header.columns - 1; i > 0; i --) {
			px = Header.origin_x + ((double) i * Header.column_spacing);
			tx = (px - eminx) / xextents;

			if (UseQuads)
				glBegin(GL_QUAD_STRIP);
			else
				glBegin(GL_TRIANGLE_STRIP);

			if (jstep > 0) {
				py = Header.origin_y;
				for (j = 0; j < Header.points; j ++) {
					ty = (py - eminy) / yextents;

					glNormal3f(PointNormals[i - 1] [j].x,
							   PointNormals[i - 1] [j].y,	
							   PointNormals[i - 1] [j].z);
					
					glTexCoord2d(tx - txratio, ty);
					glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
				
					glTexCoord2d(tx, ty);
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py += Header.point_spacing;
				}
			}
			else {
				py = Header.origin_y + Height();
				for (j = Header.points - 1; j >= 0; j --) {
					ty = (py - eminy) / yextents;

					glNormal3f(PointNormals[i] [j].x,
							   PointNormals[i] [j].y,	
							   PointNormals[i] [j].z);
				
					glTexCoord2d(tx, ty);
					glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

					glNormal3f(PointNormals[i - 1] [j].x,
							   PointNormals[i - 1] [j].y,	
							   PointNormals[i - 1] [j].z);
					
					glTexCoord2d(tx - txratio, ty);
					glVertex3d(SCALEDX(px + Header.column_spacing), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

					py -= Header.point_spacing;
				}
			}
			glEnd();
		}
	}
}

void DTM3D::DrawGroundSurfaceWithTiledTexture(double eminx, double eminy, double emaxx, double emaxy, double ve, BOOL UseQuads)
{
	// general approach is to build a new model on the fly to fit the tile extents and use about the same cell size
	// as the original model.  normals for the closest gridpoint in the original model are used instead of computing
	// new normals.  new normals would require allocating memory for the new model, getting the elevations, then computing 
	// and smoothing normals...not a bad thing but given that we already have the model loaded why take the time.
	//
	// potential problem is that the new model is not exactly the same as the original model so tree elevations obtained from 
	// the original model may not exactly match the new tiled model...error should be small given floating point elevations
	// when interpolating.  Offset of tree base into the ground by the stem radius should cover up the problem

	int i;
	int j;
	double px, py;
	double tx, ty;
	double txratio;
	double tyratio;
	double xextents;
	double yextents;
	double temp_column_spacing;
	double temp_point_spacing;
	double temp_elevation;
	int temp_columns, temp_points;
	int celli, cellj;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	// calculate tile cell size
	temp_columns = (int) ((emaxx - eminx) / Header.column_spacing) + 1;
	temp_column_spacing = (emaxx - eminx) / (double) (temp_columns - 1);
	temp_points = (int) ((emaxy - eminy) / Header.point_spacing) + 1;
	temp_point_spacing = (emaxy - eminy) / (double) (temp_points - 1);

	xextents = emaxx - eminx;
	yextents = emaxy - eminy;
	txratio = temp_column_spacing / xextents;
	tyratio = temp_point_spacing / yextents;

	px = eminx;
	tx = 0.0;
	for (i = 0; i < temp_columns - 1; i ++) {
		py = eminy;

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		ty = 0.0;
		for (j = 0; j < temp_points; j ++) {

			// get elevation and nearest cell
			temp_elevation = InterpolateElev(px, py, FALSE);
			celli = (int) floor((px - Header.origin_x) / Header.column_spacing);
			cellj = (int) floor((py - Header.origin_y) / Header.point_spacing);

			glNormal3f(PointNormals[celli] [cellj].x,
					   PointNormals[celli] [cellj].y,	
					   PointNormals[celli] [cellj].z);
		
			glTexCoord2d(tx, ty);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ((temp_elevation - Header.min_z) * ve + Header.min_z));

			// get elevation and nearest cell
			temp_elevation = InterpolateElev(px + temp_column_spacing, py, FALSE);
			celli = (int) floor(((px + temp_column_spacing) - Header.origin_x) / Header.column_spacing);
			cellj = (int) floor((py - Header.origin_y) / Header.point_spacing);

			glNormal3f(PointNormals[celli] [cellj].x,
					   PointNormals[celli] [cellj].y,	
					   PointNormals[celli] [cellj].z);
			
			glTexCoord2d(tx + txratio, ty);
			glVertex3d(SCALEDX(px + temp_column_spacing), SCALEDY(py), SCALEDZ((temp_elevation - Header.min_z) * ve + Header.min_z));

			py += temp_point_spacing;
			ty += tyratio;
		}
		glEnd();

		px += temp_column_spacing;
		tx += txratio;
	}
}

void DTM3D::DrawGroundSurfaceWithTextureFast(double eminx, double eminy, double emaxx, double emaxy, int skip, double ve, BOOL UseQuads)
{
	if (skip == 1) {
		DrawGroundSurfaceWithTexture(eminx, eminy, emaxx, emaxy, ve, UseQuads);
		return;
	}

	int i;
	int j;
	double px, py;
	double tx, ty;
	double txratio;
	double xinc, yinc;

	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	// adjust for mismatched XY and elevation units
	if (Header.xy_units != Header.z_units) {
		if (Header.xy_units == 0 && Header.z_units == 1)
			ve *= 3.28084;
		else if (Header.xy_units == 1 && Header.z_units == 0)
			ve *= 0.3048;
	}

	xinc = Header.column_spacing * (double) skip;
	yinc = Header.point_spacing * (double) skip;
	txratio = (Header.column_spacing * (double) skip) / (emaxx - eminx);
	for (i = 0; i < Header.columns - skip; i += skip) {
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;
		tx = (px - eminx) / (emaxx - eminx);

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points - skip; j += skip) {
			ty = (py - eminy) / (emaxy - eminy);

			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			
			glTexCoord2d(tx, ty);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + skip] [j].x,
					   PointNormals[i + skip] [j].y,	
					   PointNormals[i + skip] [j].z);
			
			glTexCoord2d(tx + txratio, ty);
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + skip, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += yinc;
		}
		// send last point in the column
		if (Header.points % skip) {
			py = Header.origin_y + (double) Header.points * Header.point_spacing;
			ty = (py - eminy) / (emaxy - eminy);
			glNormal3f(PointNormals[i] [Header.points - 1].x,
					   PointNormals[i] [Header.points - 1].y,	
					   PointNormals[i] [Header.points - 1].z);
			
			glTexCoord2d(tx, ty);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[i + skip] [Header.points - 1].x,
					   PointNormals[i + skip] [Header.points - 1].y,	
					   PointNormals[i + skip] [Header.points - 1].z);
			
			glTexCoord2d(tx + txratio, ty);
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i + skip, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));
		}
		glEnd();
	}
	if (Header.columns % skip) {
//		i -= skip;
		px = Header.origin_x + ((double) i * Header.column_spacing);
		py = Header.origin_y;
		tx = (px - eminx) / (emaxx - eminx);

		if (UseQuads)
			glBegin(GL_QUAD_STRIP);
		else
			glBegin(GL_TRIANGLE_STRIP);

		for (j = 0; j < Header.points - skip; j += skip) {
			ty = (py - eminy) / (emaxy - eminy);
			glNormal3f(PointNormals[i] [j].x,
					   PointNormals[i] [j].y,	
					   PointNormals[i] [j].z);
			
			glTexCoord2d(tx, ty);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, j, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[Header.columns - 1] [j].x,
					   PointNormals[Header.columns - 1] [j].y,	
					   PointNormals[Header.columns - 1] [j].z);
			
			glTexCoord2d(tx + txratio, ty);
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(Header.columns - 1, j, TRUE) - Header.min_z) * ve + Header.min_z));

			py += yinc;
		}
		// send last point in the column
		if (Header.points % skip) {
			py = Header.origin_y + (double) Header.points * Header.point_spacing;
			ty = (py - eminy) / (emaxy - eminy);
			glNormal3f(PointNormals[i] [Header.points - 1].x,
					   PointNormals[i] [Header.points - 1].y,	
					   PointNormals[i] [Header.points - 1].z);
			
			glTexCoord2d(tx, ty);
			glVertex3d(SCALEDX(px), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(i, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));

			glNormal3f(PointNormals[Header.columns - 1] [Header.points - 1].x,
					   PointNormals[Header.columns - 1] [Header.points - 1].y,	
					   PointNormals[Header.columns - 1] [Header.points - 1].z);
			
			glTexCoord2d(tx + txratio, ty);
			glVertex3d(SCALEDX(px + xinc), SCALEDY(py), SCALEDZ(((double) ReadInternalElevationValue(Header.columns - 1, Header.points - 1, TRUE) - Header.min_z) * ve + Header.min_z));
		}
		glEnd();
	}
}


void DTM3D::SetNormalVector(int r, int c)
{
	if (!Valid || !HaveElevations || !HaveNormals)
		return;

	glNormal3f(PointNormals[c] [r].x,
			   PointNormals[c] [r].y,	
			   PointNormals[c] [r].z);
}

void DTM3D::AverageNormals()
{
	VECTOR3f p;
	float magnitude;

	if (HaveNormals) {
		for (long i = 1; i < Header.columns - 2; i ++) {
			for (long j = 1; j < Header.points - 2; j ++) {
				// use points on "+"
				p.x = (PointNormals[i - 1] [j].x + PointNormals[i] [j + 1].x + PointNormals[i] [j - 1].x + PointNormals[i + 1] [j].x) / 4.0f;
				p.y = (PointNormals[i - 1] [j].y + PointNormals[i] [j + 1].y + PointNormals[i] [j - 1].y + PointNormals[i + 1] [j].y) / 4.0f;
				p.z = (PointNormals[i - 1] [j].z + PointNormals[i] [j + 1].z + PointNormals[i] [j - 1].z + PointNormals[i + 1] [j].z) / 4.0f;

				// use diagonal points on "x"
//				p.x = (PointNormals[i - 1] [j - 1].x + PointNormals[i - 1] [j + 1].x + PointNormals[i + 1] [j - 1].x + PointNormals[i + 1] [j + 1].x) / 4.0f;
//				p.y = (PointNormals[i - 1] [j - 1].y + PointNormals[i - 1] [j + 1].y + PointNormals[i + 1] [j - 1].y + PointNormals[i + 1] [j + 1].y) / 4.0f;
//				p.z = (PointNormals[i - 1] [j - 1].z + PointNormals[i - 1] [j + 1].z + PointNormals[i + 1] [j - 1].z + PointNormals[i + 1] [j + 1].z) / 4.0f;

				// normalize
				magnitude = (float) sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
				PointNormals[i] [j].x = p.x / magnitude;
				PointNormals[i] [j].y = p.y / magnitude;
				PointNormals[i] [j].z = p.z / magnitude;
			}
		}
	}
}

void DTM3D::DrawDiskWireframe(BOOL apron, double ve)
{
	double x, y, z;
	double xp, yp, zp;
	double radius;
	double BaseElev = -2.0;

	if (!Valid || !HaveElevations)
		return;

	SetVerticalExaggeration(ve);

	// draw triangle fan
	radius = Width() / 2.0;
	x = Header.origin_x + Width() / 2.0;
	y = Header.origin_y + Height() / 2.0;
	z = InterpolateElev(x, y);
	glBegin(GL_LINE_STRIP);
		for (int i = 0; i <= 360; i += SECTORSTEP) {
			xp = x + cos((double) i * D2R) * radius;
			yp = y + sin((double) i * D2R) * radius;
			zp = InterpolateElev(xp, yp);
			glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp));
		}
	glEnd();

	glBegin(GL_LINES);
		for (i = 0; i <= 360; i += SECTORSTEP) {
			xp = x + cos((double) i * D2R) * radius;
			yp = y + sin((double) i * D2R) * radius;
			zp = InterpolateElev(xp, yp);
			glVertex3d(SCALEDX(x), SCALEDY(y), SCALEDZ(z));
			glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp));
		}
	glEnd();

	if (apron) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glBegin(GL_QUAD_STRIP);
			for (i = 0; i <= 360; i += SECTORSTEP) {
				xp = x + cos((double) i * D2R) * radius;
				yp = y + sin((double) i * D2R) * radius;
				zp = InterpolateElev(xp, yp);
				glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp));
				glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp + BaseElev));
			}
		glEnd();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void DTM3D::DrawDiskSolid(double ve)
{
	double x, y, z;
	double xp, yp, zp;
	double radius;
	double BaseElev = -2.0;

	if (!Valid || !HaveElevations)
		return;

	SetVerticalExaggeration(ve);

	glNormal3f(0.0f, 1.0f, 0.0f);

	// draw triangle fan
	radius = Width() / 2.0;
	x = Header.origin_x + Width() / 2.0;
	y = Header.origin_y + Height() / 2.0;
	z = InterpolateElev(x, y);
	glBegin(GL_TRIANGLE_FAN);
		glVertex3d(SCALEDX(x), SCALEDY(y), SCALEDZ(z));
		for (int i = 0; i <= 360; i += SECTORSTEP) {
			xp = x + cos((double) i * D2R) * radius;
			yp = y + sin((double) i * D2R) * radius;
			zp = InterpolateElev(xp, yp);
			glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp));
		}
	glEnd();
}

void DTM3D::DrawDiskBase(double ve)
{
	double x, y, z;
	double xp, yp, zp;
	double radius;
	double BaseElev = -2.0;

	if (!Valid || !HaveElevations)
		return;

	SetVerticalExaggeration(ve);

	// draw triangle fan
	radius = Width() / 2.0;
	x = Header.origin_x + Width() / 2.0;
	y = Header.origin_y + Height() / 2.0;
	z = InterpolateElev(x, y);
	glBegin(GL_QUAD_STRIP);
		for (int i = 0; i <= 360; i += SECTORSTEP) {
			xp = x + cos((double) i * D2R) * radius;
			yp = y + sin((double) i * D2R) * radius;
			zp = InterpolateElev(xp, yp);
			glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp));
			glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp + BaseElev));
		}
	glEnd();
}

void DTM3D::DrawDiskSolidWithTexture(double eminx, double eminy, double emaxx, double emaxy, double ve)
{
	double x, y, z;
	double xp, yp, zp;
	double radius;
	double BaseElev = -2.0;

	if (!Valid || !HaveElevations)
		return;

	SetVerticalExaggeration(ve);

	glNormal3f(0.0f, 1.0f, 0.0f);

	// draw triangle fan
	radius = Width() / 2.0;
	x = Header.origin_x + Width() / 2.0;
	y = Header.origin_y + Height() / 2.0;
	z = InterpolateElev(x, y);
	glBegin(GL_TRIANGLE_FAN);
		glTexCoord2d((x - Header.origin_x) / Width(), (y - Header.origin_y) / Height());
		glVertex3d(SCALEDX(x), SCALEDY(y), SCALEDZ(z));
		for (int i = 0; i <= 360; i += SECTORSTEP) {
			xp = x + cos((double) i * D2R) * radius;
			yp = y + sin((double) i * D2R) * radius;
			zp = InterpolateElev(xp, yp);
			glTexCoord2d((xp - Header.origin_x) / Width(), (yp - Header.origin_y) / Height());
			glVertex3d(SCALEDX(xp), SCALEDY(yp), SCALEDZ(zp));
		}
	glEnd();
}

void DTM3D::CalculateCornerDistances()
{
	// compute distance from corners of model to point...store in m_CornerDistances[]
	if (Valid && HaveElevations) {
		// get projection matrices
		GLdouble mm[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, mm);

		GLdouble pm[16];
		glGetDoublev(GL_PROJECTION_MATRIX, pm);

		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);

		double cx, cy, cz;
		double px, py, pz;
		cx = Header.origin_x;
		cy = Header.origin_y;
		cz = ReadInternalElevationValue(0, 0);
		gluProject(cx, cy, cz, mm, pm, vp, &px, &py, &pz);
		m_CornerDistance[0] = pz;

		cx = Header.origin_x + Width();
		cy = Header.origin_y;
		cz = ReadInternalElevationValue(Header.columns - 1, 0);
		gluProject(cx, cy, cz, mm, pm, vp, &px, &py, &pz);
		m_CornerDistance[1] = pz;

		cx = Header.origin_x + Width();
		cy = Header.origin_y + Height();
		cz = ReadInternalElevationValue(Header.columns - 1, Header.points - 1);
		gluProject(cx, cy, cz, mm, pm, vp, &px, &py, &pz);
		m_CornerDistance[2] = pz;

		cx = Header.origin_x;
		cy = Header.origin_y + Height();
		cz = ReadInternalElevationValue(0, Header.points - 1);
		gluProject(cx, cy, cz, mm, pm, vp, &px, &py, &pz);
		m_CornerDistance[3] = pz;
	}
	else {
		m_CornerDistance[0] = m_CornerDistance[1] = m_CornerDistance[2] = m_CornerDistance[3] = 0.0;
	}
}

int DTM3D::GetFarthestCorner()
{
	double maxdist = 0.0;
	int index = 0;

	for (int i = 0; i < 4; i ++) {
		if (m_CornerDistance[i] > maxdist) {
			maxdist = m_CornerDistance[i];
			index = i;
		}
	}
	return(index);
}

BOOL DTM3D::CalculateCellDistances()
{
	// allocate space for structures
	if (Valid && HaveElevations) {
		if (!m_CellDistances)
			m_CellDistances = (CELLDISTANCE*) new CELLDISTANCE[(Rows() - 1) * (Columns() - 1)];

		if (m_CellDistances) {
			// get projection matrices
			GLdouble mm[16];
			glGetDoublev(GL_MODELVIEW_MATRIX, mm);

			GLdouble pm[16];
			glGetDoublev(GL_PROJECTION_MATRIX, pm);

			GLint vp[4];
			glGetIntegerv(GL_VIEWPORT, vp);

			double cx, cy, cz;
			double px, py, pz;
			int index = 0;
			cx = Header.origin_x + Header.column_spacing / 2.0;
			for (int i = 0; i < Columns() - 1; i ++) {
				cy = Header.origin_y + Header.point_spacing / 2.0;
				for (int j = 0; j < Rows() - 1; j ++) {
					cz = InterpolateElev(cx, cy);
					gluProject(cx, cy, cz, mm, pm, vp, &px, &py, &pz);
					m_CellDistances[index].CellIndex = i * (Rows() - 1) + j;
					m_CellDistances[index].DistanceValue = pz;

					index ++;
					cy += Header.point_spacing;
				}
				cx += Header.column_spacing;
			}

			return(TRUE);
		}
	}
	return(FALSE);
}

void DTM3D::SetScalingForRendering(double minx, double miny, double minz, double maxx, double maxy, double maxz)
{
	m_MinScalingX = minx;
	m_MinScalingY = miny;
	m_MinScalingZ = minz;
	m_MaxScalingX = maxx;
	m_MaxScalingY = maxy;
	m_MaxScalingZ = maxz;

	m_UseScaling = TRUE;

	m_ScalingXRange = m_MaxScalingX - m_MinScalingX;
	m_ScalingYRange = m_MaxScalingY - m_MinScalingY;
	m_ScalingZRange = m_MaxScalingZ - m_MinScalingZ;
}

void DTM3D::Empty()
{
	// get rid of normals
	if (HaveNormals) {
		for (int i = 0; i < Header.columns; i ++)
			delete [] PointNormals[i];

		delete [] PointNormals;

		HaveNormals = FALSE;
	}

	// delete cell list
	if (m_CellDistances) {
		delete [] m_CellDistances;
		m_CellDistances = NULL;
	}
}
