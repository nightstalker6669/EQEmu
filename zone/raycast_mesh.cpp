/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "../common/housing/curved_mount_support.h"
#include <mutex>
#include <tuple>
#include <memory>
#include "../common/housing/mount_surface_patch.h"
#include "../common/housing/planar_mount_patch.h"
#include "../common/housing/exterior_exit_geometry.h"

#include "raycast_mesh.h"

#include "common/eqemu_logsys.h"
#include "common/memory/ksm.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

// This code snippet allows you to create an axis aligned bounding volume tree for a triangle mesh so that you can do
// high-speed raycasting.
//
// There are much better implementations of this available on the internet.  In particular I recommend that you use
// OPCODE written by Pierre Terdiman.
// @see: http://www.codercorner.com/Opcode.htm
//
// OPCODE does a whole lot more than just raycasting, and is a rather significant amount of source code.
//
// I am providing this code snippet for the use case where you *only* want to do quick and dirty optimized raycasting.
// I have not done performance testing between this version and OPCODE; so I don't know how much slower it is.  However,
// anytime you switch to using a spatial data structure for raycasting, you increase your performance by orders and orders
// of magnitude; so this implementation should work fine for simple tools and utilities.
//
// It also serves as a nice sample for people who are trying to learn the algorithm of how to implement AABB trees.
// AABB = Axis Aligned Bounding Volume trees.
//
// http://www.cgal.org/Manual/3.5/doc_html/cgal_manual/AABB_tree/Chapter_main.html
//
//
// This code snippet was written by John W. Ratcliff on August 18, 2011 and released under the MIT. license.
//
// mailto:jratcliffscarab@gmail.com
//
// The official source can be found at:  http://code.google.com/p/raycastmesh/
//
//

#pragma warning(disable:4100)

namespace RAYCAST_MESH
{

typedef std::vector<RmUint32, PageAlignedAllocator<RmUint32>> TriVector;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
*	A method to compute a ray-AABB intersection.
*	Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990
*	Optimized code by Pierre Terdiman, 2000 (~20-30% faster on my Celeron 500)
*	Epsilon value added by Klaus Hartmann. (discarding it saves a few cycles only)
*
*	Hence this version is faster as well as more robust than the original one.
*
*	Should work provided:
*	1) the integer representation of 0.0f is 0x00000000
*	2) the sign bit of the RmReal is the most significant one
*
*	Report bugs: p.terdiman@codercorner.com
*
*	\param		aabb		[in] the axis-aligned bounding box
*	\param		origin		[in] ray origin
*	\param		dir			[in] ray direction
*	\param		coord		[out] impact coordinates
*	\return		true if ray intersects AABB
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define RAYAABB_EPSILON 0.00001f
//! Integer representation of a RmRealing-point value.
#define IR(x)	((RmUint32&)x)

bool intersectRayAABB(const RmReal MinB[3],const RmReal MaxB[3],const RmReal origin[3],const RmReal dir[3],RmReal coord[3])
{
	bool Inside = true;
	RmReal MaxT[3];
	MaxT[0]=MaxT[1]=MaxT[2]=-1.0f;

	// Find candidate planes.
	for(RmUint32 i=0;i<3;i++)
	{
		if(origin[i] < MinB[i])
		{
			coord[i]	= MinB[i];
			Inside		= false;

			// Calculate T distances to candidate planes
			if(IR(dir[i]))	MaxT[i] = (MinB[i] - origin[i]) / dir[i];
		}
		else if(origin[i] > MaxB[i])
		{
			coord[i]	= MaxB[i];
			Inside		= false;

			// Calculate T distances to candidate planes
			if(IR(dir[i]))	MaxT[i] = (MaxB[i] - origin[i]) / dir[i];
		}
	}

	// Ray origin inside bounding box
	if(Inside)
	{
		coord[0] = origin[0];
		coord[1] = origin[1];
		coord[2] = origin[2];
		return true;
	}

	// Get largest of the maxT's for final choice of intersection
	RmUint32 WhichPlane = 0;
	if(MaxT[1] > MaxT[WhichPlane])	WhichPlane = 1;
	if(MaxT[2] > MaxT[WhichPlane])	WhichPlane = 2;

	// Check final candidate actually inside box
	if(IR(MaxT[WhichPlane])&0x80000000) return false;

	for(RmUint32 i=0;i<3;i++)
	{
		if(i!=WhichPlane)
		{
			coord[i] = origin[i] + MaxT[WhichPlane] * dir[i];
			#ifdef RAYAABB_EPSILON
			if(coord[i] < MinB[i] - RAYAABB_EPSILON || coord[i] > MaxB[i] + RAYAABB_EPSILON)	return false;
			#else
			if(coord[i] < MinB[i] || coord[i] > MaxB[i])	return false;
			#endif
		}
	}
	return true;	// ray hits box
}




bool intersectLineSegmentAABB(const RmReal bmin[3],const RmReal bmax[3],const RmReal p1[3],const RmReal dir[3],RmReal &dist,RmReal intersect[3])
{
	bool ret = false;

	if ( dist > RAYAABB_EPSILON )
	{
		ret = intersectRayAABB(bmin,bmax,p1,dir,intersect);
		if ( ret )
		{
			RmReal dx = p1[0]-intersect[0];
			RmReal dy = p1[1]-intersect[1];
			RmReal dz = p1[2]-intersect[2];
			RmReal d = dx*dx+dy*dy+dz*dz;
			if ( d < dist*dist )
			{
				dist = sqrtf(d);
			}
			else
			{
				ret = false;
			}
		}
	}
	return ret;
}

/* a = b - c */
#define vector(a,b,c) \
	(a)[0] = (b)[0] - (c)[0];	\
	(a)[1] = (b)[1] - (c)[1];	\
	(a)[2] = (b)[2] - (c)[2];

#define innerProduct(v,q) \
	((v)[0] * (q)[0] + \
	(v)[1] * (q)[1] + \
	(v)[2] * (q)[2])

#define crossProduct(a,b,c) \
	(a)[0] = (b)[1] * (c)[2] - (c)[1] * (b)[2]; \
	(a)[1] = (b)[2] * (c)[0] - (c)[2] * (b)[0]; \
	(a)[2] = (b)[0] * (c)[1] - (c)[0] * (b)[1];


static inline bool rayIntersectsTriangle(const RmReal *p,const RmReal *d,const RmReal *v0,const RmReal *v1,const RmReal *v2,RmReal &t)
{
	RmReal e1[3],e2[3],h[3],s[3],q[3];
	RmReal a,f,u,v;

	vector(e1,v1,v0);
	vector(e2,v2,v0);
	crossProduct(h,d,e2);
	a = innerProduct(e1,h);

	if (a > -0.00001 && a < 0.00001)
		return(false);

	f = 1/a;
	vector(s,p,v0);
	u = f * (innerProduct(s,h));

	if (u < 0.0 || u > 1.0)
		return(false);

	crossProduct(q,s,e1);
	v = f * innerProduct(d,q);
	if (v < 0.0 || u + v > 1.0)
		return(false);
	// at this stage we can compute t to find out where
	// the intersection point is on the line
	t = f * innerProduct(e2,q);
	if (t > 0) // ray intersection
		return(true);
	else // this means that there is a line intersection
		// but not a ray intersection
		return (false);
}

static RmReal computePlane(const RmReal *A,const RmReal *B,const RmReal *C,RmReal *n) // returns D
{
	RmReal vx = (B[0] - C[0]);
	RmReal vy = (B[1] - C[1]);
	RmReal vz = (B[2] - C[2]);

	RmReal wx = (A[0] - B[0]);
	RmReal wy = (A[1] - B[1]);
	RmReal wz = (A[2] - B[2]);

	RmReal vw_x = vy * wz - vz * wy;
	RmReal vw_y = vz * wx - vx * wz;
	RmReal vw_z = vx * wy - vy * wx;

	RmReal mag = sqrt((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

	if ( mag < 0.000001f )
	{
		mag = 0;
	}
	else
	{
		mag = 1.0f/mag;
	}

	RmReal x = vw_x * mag;
	RmReal y = vw_y * mag;
	RmReal z = vw_z * mag;


	RmReal D = 0.0f - ((x*A[0])+(y*A[1])+(z*A[2]));

	n[0] = x;
	n[1] = y;
	n[2] = z;

	return D;
}


#define TRI_EOF 0xFFFFFFFF

enum AxisAABB
{
	AABB_XAXIS,
	AABB_YAXIS,
	AABB_ZAXIS
};

enum ClipCode
{
	CC_MINX   =       (1<<0),
	CC_MAXX  =       (1<<1),
	CC_MINY   =       (1<<2),
	CC_MAXY	 =       (1<<3),
	CC_MINZ  =       (1<<4),
	CC_MAXZ   =       (1<<5),
};


class BoundsAABB
{
public:


	void setMin(const RmReal *v)
	{
		mMin[0] = v[0];
		mMin[1] = v[1];
		mMin[2] = v[2];
	}

	void setMax(const RmReal *v)
	{
		mMax[0] = v[0];
		mMax[1] = v[1];
		mMax[2] = v[2];
	}

	void setMin(RmReal x,RmReal y,RmReal z)
	{
		mMin[0] = x;
		mMin[1] = y;
		mMin[2] = z;
	}

	void setMax(RmReal x,RmReal y,RmReal z)
	{
		mMax[0] = x;
		mMax[1] = y;
		mMax[2] = z;
	}

	void include(const RmReal *v)
	{
		if ( v[0] < mMin[0] ) mMin[0] = v[0];
		if ( v[1] < mMin[1] ) mMin[1] = v[1];
		if ( v[2] < mMin[2] ) mMin[2] = v[2];

		if ( v[0] > mMax[0] ) mMax[0] = v[0];
		if ( v[1] > mMax[1] ) mMax[1] = v[1];
		if ( v[2] > mMax[2] ) mMax[2] = v[2];
	}

	void getCenter(RmReal *center) const
	{
		center[0] = (mMin[0]+mMax[0])*0.5f;
		center[1] = (mMin[1]+mMax[1])*0.5f;
		center[2] = (mMin[2]+mMax[2])*0.5f;
	}

	bool intersects(const BoundsAABB &b) const
	{
		if ((mMin[0] > b.mMax[0]) || (b.mMin[0] > mMax[0])) return false;
		if ((mMin[1] > b.mMax[1]) || (b.mMin[1] > mMax[1])) return false;
		if ((mMin[2] > b.mMax[2]) || (b.mMin[2] > mMax[2])) return false;
		return true;
	}

	bool containsTriangle(const RmReal *p1,const RmReal *p2,const RmReal *p3) const
	{
		BoundsAABB b;
		b.setMin(p1);
		b.setMax(p1);
		b.include(p2);
		b.include(p3);
		return intersects(b);
	}

	bool containsTriangleExact(const RmReal *p1,const RmReal *p2,const RmReal *p3,RmUint32 &orCode) const
	{
		bool ret = false;

		RmUint32 andCode;
		orCode = getClipCode(p1,p2,p3,andCode);
		if ( andCode == 0 )
		{
			ret = true;
		}

		return ret;
	}

	inline RmUint32 getClipCode(const RmReal *p1,const RmReal *p2,const RmReal *p3,RmUint32 &andCode) const
	{
		andCode = 0xFFFFFFFF;
		RmUint32 c1 = getClipCode(p1);
		RmUint32 c2 = getClipCode(p2);
		RmUint32 c3 = getClipCode(p3);
		andCode&=c1;
		andCode&=c2;
		andCode&=c3;
		return c1|c2|c3;
	}

	inline RmUint32 getClipCode(const RmReal *p) const
	{
		RmUint32 ret = 0;

		if ( p[0] < mMin[0] )
		{
			ret|=CC_MINX;
		}
		else if ( p[0] > mMax[0] )
		{
			ret|=CC_MAXX;
		}

		if ( p[1] < mMin[1] )
		{
			ret|=CC_MINY;
		}
		else if ( p[1] > mMax[1] )
		{
			ret|=CC_MAXY;
		}

		if ( p[2] < mMin[2] )
		{
			ret|=CC_MINZ;
		}
		else if ( p[2] > mMax[2] )
		{
			ret|=CC_MAXZ;
		}

		return ret;
	}

	inline void clamp(const BoundsAABB &aabb)
	{
		if ( mMin[0] < aabb.mMin[0] ) mMin[0] = aabb.mMin[0];
		if ( mMin[1] < aabb.mMin[1] ) mMin[1] = aabb.mMin[1];
		if ( mMin[2] < aabb.mMin[2] ) mMin[2] = aabb.mMin[2];
		if ( mMax[0] > aabb.mMax[0] ) mMax[0] = aabb.mMax[0];
		if ( mMax[1] > aabb.mMax[1] ) mMax[1] = aabb.mMax[1];
		if ( mMax[2] > aabb.mMax[2] ) mMax[2] = aabb.mMax[2];
	}

	RmReal		mMin[3];
	RmReal		mMax[3];
};


class NodeAABB;

class NodeInterface
{
public:
	virtual NodeAABB * getNode(void) = 0;
	virtual void getFaceNormal(RmUint32 tri,RmReal *faceNormal) = 0;
};





	class NodeAABB
	{
	public:
		NodeAABB(void)
		{
			mLeft = NULL;
			mRight = NULL;
			mLeafTriangleIndex= TRI_EOF;
		}

		NodeAABB(RmUint32 vcount,const RmReal *vertices,RmUint32 tcount,RmUint32 *indices,
			RmUint32 maxDepth,	// Maximum recursion depth for the triangle mesh.
			RmUint32 minLeafSize,	// minimum triangles to treat as a 'leaf' node.
			RmReal	minAxisSize,
			NodeInterface *callback,
			TriVector &leafTriangles)	// once a particular axis is less than this size, stop sub-dividing.

		{
			mLeft = NULL;
			mRight = NULL;
			mLeafTriangleIndex = TRI_EOF;
			TriVector triangles;
			triangles.reserve(tcount);
			for (RmUint32 i=0; i<tcount; i++)
			{
				triangles.push_back(i);
			}
			mBounds.setMin( vertices );
			mBounds.setMax( vertices );
			const RmReal *vtx = vertices+3;
			for (RmUint32 i=1; i<vcount; i++)
			{
				mBounds.include( vtx );
				vtx+=3;
			}
			split(triangles,vcount,vertices,tcount,indices,0,maxDepth,minLeafSize,minAxisSize,callback,leafTriangles);
		}

		NodeAABB(const BoundsAABB &aabb)
		{
			mBounds = aabb;
			mLeft = NULL;
			mRight = NULL;
			mLeafTriangleIndex = TRI_EOF;
		}

		~NodeAABB(void)
		{
		}

		// here is where we split the mesh..
		void split(const TriVector &triangles,
			RmUint32 vcount,
			const RmReal *vertices,
			RmUint32 tcount,
			const RmUint32 *indices,
			RmUint32 depth,
			RmUint32 maxDepth,	// Maximum recursion depth for the triangle mesh.
			RmUint32 minLeafSize,	// minimum triangles to treat as a 'leaf' node.
			RmReal	minAxisSize,
			NodeInterface *callback,
			TriVector &leafTriangles)	// once a particular axis is less than this size, stop sub-dividing.

		{
			// Find the longest axis of the bounding volume of this node
			RmReal dx = mBounds.mMax[0] - mBounds.mMin[0];
			RmReal dy = mBounds.mMax[1] - mBounds.mMin[1];
			RmReal dz = mBounds.mMax[2] - mBounds.mMin[2];

			AxisAABB axis = AABB_XAXIS;
			RmReal laxis = dx;

			if ( dy > dx )
			{
				axis = AABB_YAXIS;
				laxis = dy;
			}

			if ( dz > dx && dz > dy )
			{
				axis = AABB_ZAXIS;
				laxis = dz;
			}

			RmUint32 count = triangles.size();

			// if the number of triangles is less than the minimum allowed for a leaf node or...
			// we have reached the maximum recursion depth or..
			// the width of the longest axis is less than the minimum axis size then...
			// we create the leaf node and copy the triangles into the leaf node triangle array.
			if ( count < minLeafSize || depth >= maxDepth || laxis < minAxisSize )
			{
				// Copy the triangle indices into the leaf triangles array
				mLeafTriangleIndex = leafTriangles.size(); // assign the array start location for these leaf triangles.
				leafTriangles.push_back(count);
				for (auto i = triangles.begin(); i != triangles.end(); ++i) {
					RmUint32 tri = *i;
					leafTriangles.push_back(tri);
				}
			}
			else
			{
				RmReal center[3];
				mBounds.getCenter(center);
				BoundsAABB b1,b2;
				splitRect(axis,mBounds,b1,b2,center);

				// Compute two bounding boxes based upon the split of the longest axis

				BoundsAABB leftBounds,rightBounds;

				TriVector leftTriangles;
				TriVector rightTriangles;


				// Create two arrays; one of all triangles which intersect the 'left' half of the bounding volume node
				// and another array that includes all triangles which intersect the 'right' half of the bounding volume node.
				for (auto i = triangles.begin(); i != triangles.end(); ++i) {

					RmUint32 tri = (*i);

					{
						RmUint32 i1 = indices[tri*3+0];
						RmUint32 i2 = indices[tri*3+1];
						RmUint32 i3 = indices[tri*3+2];

						const RmReal *p1 = &vertices[i1*3];
						const RmReal *p2 = &vertices[i2*3];
						const RmReal *p3 = &vertices[i3*3];

						RmUint32 addCount = 0;
						RmUint32 orCode=0xFFFFFFFF;
						if ( b1.containsTriangleExact(p1,p2,p3,orCode))
						{
							addCount++;
							if ( leftTriangles.empty() )
							{
								leftBounds.setMin(p1);
								leftBounds.setMax(p1);
							}
							leftBounds.include(p1);
							leftBounds.include(p2);
							leftBounds.include(p3);
							leftTriangles.push_back(tri); // Add this triangle to the 'left triangles' array and revise the left triangles bounding volume
						}
						// if the orCode is zero; meaning the triangle was fully self-contiained int he left bounding box; then we don't need to test against the right
						if ( orCode && b2.containsTriangleExact(p1,p2,p3,orCode))
						{
							addCount++;
							if ( rightTriangles.empty() )
							{
								rightBounds.setMin(p1);
								rightBounds.setMax(p1);
							}
							rightBounds.include(p1);
							rightBounds.include(p2);
							rightBounds.include(p3);
							rightTriangles.push_back(tri); // Add this triangle to the 'right triangles' array and revise the right triangles bounding volume.
						}
						assert( addCount );
					}
				}

				if ( !leftTriangles.empty() ) // If there are triangles in the left half then...
				{
					leftBounds.clamp(b1); // we have to clamp the bounding volume so it stays inside the parent volume.
					mLeft = callback->getNode();	// get a new AABB node
					new ( mLeft ) NodeAABB(leftBounds);		// initialize it to default constructor values.
					// Then recursively split this node.
					mLeft->split(leftTriangles,vcount,vertices,tcount,indices,depth+1,maxDepth,minLeafSize,minAxisSize,callback,leafTriangles);
				}

				if ( !rightTriangles.empty() ) // If there are triangles in the right half then..
				{
					rightBounds.clamp(b2);	// clamps the bounding volume so it stays restricted to the size of the parent volume.
					mRight = callback->getNode(); // allocate and default initialize a new node
					new ( mRight ) NodeAABB(rightBounds);
					// Recursively split this node.
					mRight->split(rightTriangles,vcount,vertices,tcount,indices,depth+1,maxDepth,minLeafSize,minAxisSize,callback,leafTriangles);
				}

			}
		}

		void splitRect(AxisAABB axis,const BoundsAABB &source,BoundsAABB &b1,BoundsAABB &b2,const RmReal *midpoint)
		{
			switch ( axis )
			{
				case AABB_XAXIS:
					{
						b1.setMin( source.mMin );
						b1.setMax( midpoint[0], source.mMax[1], source.mMax[2] );

						b2.setMin( midpoint[0], source.mMin[1], source.mMin[2] );
						b2.setMax(source.mMax);
					}
					break;
				case AABB_YAXIS:
					{
						b1.setMin(source.mMin);
						b1.setMax(source.mMax[0], midpoint[1], source.mMax[2]);

						b2.setMin(source.mMin[0], midpoint[1], source.mMin[2]);
						b2.setMax(source.mMax);
					}
					break;
				case AABB_ZAXIS:
					{
						b1.setMin(source.mMin);
						b1.setMax(source.mMax[0], source.mMax[1], midpoint[2]);

						b2.setMin(source.mMin[0], source.mMin[1], midpoint[2]);
						b2.setMax(source.mMax);
					}
					break;
			}
		}


 bool overlapsBox(const Housing::ExteriorExit::Box& box,const RmReal *vertices,RmUint32 vcount,const RmUint32 *indices,RmUint32 tcount,const TriVector& leaf,RmUint32 ignore_a=TRI_EOF,RmUint32 ignore_b=TRI_EOF,const std::vector<RmUint32>* source_list=nullptr) const {
  for(unsigned i=0;i<3;++i)if(mBounds.mMax[i]<box.minimum[i]||mBounds.mMin[i]>box.maximum[i])return false;
  if(mLeafTriangleIndex!=TRI_EOF){if(mLeafTriangleIndex>=leaf.size())return true;auto count=leaf[mLeafTriangleIndex];if(count>leaf.size()-mLeafTriangleIndex-1)return true;
   for(RmUint32 j=0;j<count;++j){auto tri=leaf[mLeafTriangleIndex+1+j];if(tri>=tcount)return true;if(tri==ignore_a||tri==ignore_b||(source_list&&std::binary_search(source_list->begin(),source_list->end(),tri)))continue;std::array<Housing::ExteriorExit::Point,3> points;for(unsigned k=0;k<3;++k){auto index=indices[3*tri+k];if(index>=vcount)return true;for(unsigned i=0;i<3;++i)points[k][i]=vertices[3*index+i];}if(Housing::ExteriorExit::TriangleOverlap(box,points))return true;}return false;
  }
  return (mLeft&&mLeft->overlapsBox(box,vertices,vcount,indices,tcount,leaf,ignore_a,ignore_b,source_list))||(mRight&&mRight->overlapsBox(box,vertices,vcount,indices,tcount,leaf,ignore_a,ignore_b,source_list));
 }

 bool overlapsVolume(const Housing::OrientedMount::Volume&volume,const Housing::ExteriorExit::Box&box,const RmReal *vertices,RmUint32 vcount,const RmUint32 *indices,RmUint32 tcount,const TriVector&leaf,const std::vector<RmUint32>&sources) const {
  for(unsigned i=0;i<3;++i)if(mBounds.mMax[i]<box.minimum[i]||mBounds.mMin[i]>box.maximum[i])return false;
  if(mLeafTriangleIndex!=TRI_EOF){if(mLeafTriangleIndex>=leaf.size())return true;auto count=leaf[mLeafTriangleIndex];if(count>leaf.size()-mLeafTriangleIndex-1)return true;
   for(RmUint32 j=0;j<count;++j){auto tri=leaf[mLeafTriangleIndex+1+j];if(tri>=tcount)return true;if(std::binary_search(sources.begin(),sources.end(),tri))continue;std::array<std::array<float,3>,3>points;for(unsigned k=0;k<3;++k){auto index=indices[3*tri+k];if(index>=vcount)return true;for(unsigned i=0;i<3;++i)points[k][i]=vertices[3*index+i];}if(Housing::OrientedMount::TriangleOverlap(volume,points))return true;}return false;
  }
  return (mLeft&&mLeft->overlapsVolume(volume,box,vertices,vcount,indices,tcount,leaf,sources))||(mRight&&mRight->overlapsVolume(volume,box,vertices,vcount,indices,tcount,leaf,sources));
 }

		virtual void raycast(bool &hit,
							const RmReal *from,
							const RmReal *to,
							const RmReal *dir,
							RmReal *hitLocation,
							RmReal *hitNormal,
							RmReal *hitDistance,
							const RmReal *vertices,
							const RmUint32 *indices,
							RmReal &nearestDistance,
							NodeInterface *callback,
							RmUint32 *raycastTriangles,
							RmUint32 raycastFrame,
							const TriVector &leafTriangles,
							RmUint32 &nearestTriIndex,
							RmUint32 ignoreBegin = 0, RmUint32 ignoreEnd = 0)
		{
			RmReal sect[3];
			RmReal nd = nearestDistance;
			if ( !intersectLineSegmentAABB(mBounds.mMin,mBounds.mMax,from,dir,nd,sect) )
			{
				return;
			}
			if ( mLeafTriangleIndex != TRI_EOF )
			{
				const RmUint32 *scan = &leafTriangles[mLeafTriangleIndex];
				RmUint32 count = *scan++;
				for (RmUint32 i=0; i<count; i++)
				{
					RmUint32 tri = *scan++;
					if (tri >= ignoreBegin && tri < ignoreEnd) {
						continue;
					}
					if ( raycastTriangles[tri] != raycastFrame )
					{
						raycastTriangles[tri] = raycastFrame;
						RmUint32 i1 = indices[tri*3+0];
						RmUint32 i2 = indices[tri*3+1];
						RmUint32 i3 = indices[tri*3+2];

						const RmReal *p1 = &vertices[i1*3];
						const RmReal *p2 = &vertices[i2*3];
						const RmReal *p3 = &vertices[i3*3];

						RmReal t;
						if ( rayIntersectsTriangle(from,dir,p1,p2,p3,t))
						{
							bool accept = false;
							if ( t == nearestDistance && tri < nearestTriIndex )
							{
								accept = true;
							}
							if ( t < nearestDistance || accept )
							{
								nearestDistance = t;
								if ( hitLocation )
								{
									hitLocation[0] = from[0]+dir[0]*t;
									hitLocation[1] = from[1]+dir[1]*t;
									hitLocation[2] = from[2]+dir[2]*t;
								}
								if ( hitNormal )
								{
									callback->getFaceNormal(tri,hitNormal);
								}
								if ( hitDistance )
								{
									*hitDistance = t;
								}
								nearestTriIndex = tri;
								hit = true;
							}
						}
					}
				}
			}
			else
			{
				if ( mLeft )
				{
					mLeft->raycast(hit,from,to,dir,hitLocation,hitNormal,hitDistance,vertices,indices,nearestDistance,callback,raycastTriangles,raycastFrame,leafTriangles,nearestTriIndex, ignoreBegin, ignoreEnd);
				}
				if ( mRight )
				{
					mRight->raycast(hit,from,to,dir,hitLocation,hitNormal,hitDistance,vertices,indices,nearestDistance,callback,raycastTriangles,raycastFrame,leafTriangles,nearestTriIndex, ignoreBegin, ignoreEnd);
				}
			}
		}

		NodeAABB		*mLeft;			// left node
		NodeAABB		*mRight;		// right node
		BoundsAABB		mBounds;		// bounding volume of node
		RmUint32		mLeafTriangleIndex;	// if it is a leaf node; then these are the triangle indices.
	};

class MyRaycastMesh : public RaycastMesh, public NodeInterface
{
 // Source geometry is immutable for this mesh's lifetime. Cache only source
 // topology; never cache a pose/contact/coverage/collision/authorization result.
 struct MountTopology {std::vector<Housing::MountSurfacePatch::Triangle> triangles;std::vector<RmUint32> ids;};
 using MountKey=std::tuple<RmUint32,RmUint32,RmUint32,std::vector<RmUint32>>;
 mutable std::mutex mMountMutex;
 mutable std::map<MountKey,std::shared_ptr<const MountTopology>> mMountTopology;
 // Store one whitelist per inventory and share each immutable component across
 // all its seeds. A many-triangle wall must not duplicate the whole whitelist
 // and its topology once per clicked triangle. Pose coverage remains uncached.
 using PlanarSeeds=std::map<RmUint32,std::shared_ptr<const MountTopology>>;
 mutable std::map<std::vector<RmUint32>,PlanarSeeds> mPlanarTopology;
public:

 bool overlapsBox(const RmReal *minimum,const RmReal *maximum) const override {
  Housing::ExteriorExit::Box box;for(unsigned i=0;i<3;++i){box.minimum[i]=minimum[i];box.maximum[i]=maximum[i];if(!std::isfinite(minimum[i])||!std::isfinite(maximum[i])||minimum[i]>maximum[i])return true;}
  return !mRoot||mRoot->overlapsBox(box,mVertices,mVcount,mIndices,mTcount,mLeafTriangles);
 }

 bool overlapsBoxExceptTriangles(const RmReal *minimum,const RmReal *maximum,RmUint32 source_a,RmUint32 source_b) const override {
  if(source_a==source_b||source_a>=mTcount||source_b>=mTcount)return true;
  Housing::ExteriorExit::Box box;for(unsigned i=0;i<3;++i){box.minimum[i]=minimum[i];box.maximum[i]=maximum[i];if(!std::isfinite(minimum[i])||!std::isfinite(maximum[i])||minimum[i]>maximum[i])return true;}
  return !mRoot||mRoot->overlapsBox(box,mVertices,mVcount,mIndices,mTcount,mLeafTriangles,source_a,source_b);
 }

 std::shared_ptr<const MountTopology> mountTopology(RmUint32 axis,RmUint32 a,RmUint32 b,const std::vector<RmUint32>&candidates) const {
  if(!mRoot||axis>2||candidates.empty()||!std::is_sorted(candidates.begin(),candidates.end())||std::adjacent_find(candidates.begin(),candidates.end())!=candidates.end())return {};
   MountKey key{axis,a,b,candidates};std::shared_ptr<const MountTopology> prepared;
   {std::lock_guard<std::mutex> lock(mMountMutex);auto found=mMountTopology.find(key);
    if(found!=mMountTopology.end())prepared=found->second;
    else{auto entry=std::make_shared<MountTopology>();std::vector<Housing::MountSurfacePatch::Triangle> triangles;triangles.reserve(candidates.size());
     for(auto id:candidates){if(id>=mTcount)return {};Housing::MountSurfacePatch::Triangle t;t.id=id;for(unsigned j=0;j<3;++j){auto index=mIndices[3*id+j];if(index>=mVcount)return {};for(unsigned k=0;k<3;++k)t.vertices[j][k]=mVertices[3*index+k];}triangles.push_back(t);}
     entry->ids=Housing::MountSurfacePatch::Connected(triangles,axis,a,b);for(const auto&t:triangles)if(std::binary_search(entry->ids.begin(),entry->ids.end(),t.id))entry->triangles.push_back(t);
     prepared=entry;mMountTopology.emplace(std::move(key),std::move(entry));
    }
   }
  return prepared;
 }
 std::vector<RmUint32> sourceMountPatch(const RmReal *minimum,const RmReal *maximum,RmUint32 axis,RmUint32 a,RmUint32 b,const std::vector<RmUint32>& candidates) const override {
  if(!mRoot||axis>2||candidates.empty()||!std::is_sorted(candidates.begin(),candidates.end())||std::adjacent_find(candidates.begin(),candidates.end())!=candidates.end())return {};
  try{
   for(unsigned k=0;k<3;++k)if(!std::isfinite(minimum[k])||!std::isfinite(maximum[k])||minimum[k]>=maximum[k])return {};
   auto prepared=mountTopology(axis,a,b,candidates);
   if(!prepared||prepared->ids.empty())return {};std::vector<const Housing::MountSurfacePatch::Triangle*> patch;for(const auto&t:prepared->triangles)patch.push_back(&t);
   return Housing::MountSurfacePatch::Covered(patch,axis,{minimum[0],minimum[1],minimum[2]},{maximum[0],maximum[1],maximum[2]})?prepared->ids:std::vector<RmUint32>{};
  }catch(...){return {};}
 }
 bool overlapsBoxExceptTriangleList(const RmReal *minimum,const RmReal *maximum,const std::vector<RmUint32>& sources) const override {
  if(sources.empty()||!std::is_sorted(sources.begin(),sources.end())||std::adjacent_find(sources.begin(),sources.end())!=sources.end()||sources.back()>=mTcount)return true;
  Housing::ExteriorExit::Box box;for(unsigned i=0;i<3;++i){box.minimum[i]=minimum[i];box.maximum[i]=maximum[i];if(!std::isfinite(minimum[i])||!std::isfinite(maximum[i])||minimum[i]>maximum[i])return true;}
  return !mRoot||mRoot->overlapsBox(box,mVertices,mVcount,mIndices,mTcount,mLeafTriangles,TRI_EOF,TRI_EOF,&sources);
 }

 std::vector<RmUint32> sourceMountPatch(const Housing::OrientedMount::Volume&volume,RmUint32 axis,RmUint32 a,RmUint32 b,const std::vector<RmUint32>&candidates) const override {
  try{if(!Housing::OrientedMount::Valid(volume))return {};auto prepared=mountTopology(axis,a,b,candidates);if(!prepared||prepared->ids.empty())return {};std::vector<const Housing::MountSurfacePatch::Triangle*>patch;for(const auto&t:prepared->triangles)patch.push_back(&t);return Housing::MountSurfacePatch::Covered(patch,axis,volume)?prepared->ids:std::vector<RmUint32>{};}catch(...){return {};}
 }
 bool overlapsVolumeExceptTriangleList(const Housing::OrientedMount::Volume&volume,const std::vector<RmUint32>&sources) const override {
  try{if(!mRoot||!Housing::OrientedMount::Valid(volume)||sources.empty()||!std::is_sorted(sources.begin(),sources.end())||std::adjacent_find(sources.begin(),sources.end())!=sources.end()||sources.back()>=mTcount)return true;auto bounds=Housing::OrientedMount::Bounds(volume);Housing::ExteriorExit::Box box;for(unsigned k=0;k<3;++k){box.minimum[k]=bounds.first[k];box.maximum[k]=bounds.second[k];}return mRoot->overlapsVolume(volume,box,mVertices,mVcount,mIndices,mTcount,mLeafTriangles,sources);}catch(...){return true;}
 }

 RmUint32 sourceTriangleCount() const override {return mTcount;}
 Housing::CurvedMount::Proof sourceCurvedMountPatch(const Housing::OrientedMount::Volume&volume,const Housing::PlanarMount::SurfaceSeed&seed,const std::vector<Housing::CurvedMount::Entry>&entries,Housing::CurvedMount::Limits limits) const override {
  try{if(!mRoot||!Housing::OrientedMount::Valid(volume)||!std::isfinite(limits.search_margin)||limits.search_margin<=0||limits.search_margin>8||entries.empty()||entries.size()>limits.source||entries.size()>65536)return {};const auto bounds=Housing::OrientedMount::Bounds(volume);std::vector<Housing::CurvedMount::Source>sources;sources.reserve(entries.size());uint32_t prior=0;bool first=true;
   for(const auto&e:entries){if((!first&&e.id<=prior)||e.id>=mTcount||!e.side_mask||e.side_mask>3||!e.native_mode_mask||(e.native_mode_mask&~3))return {};first=false;prior=e.id;Housing::CurvedMount::Source source;source.geometry.id=e.id;source.side_mask=e.side_mask;source.native_mode_mask=e.native_mode_mask;for(unsigned j=0;j<3;++j){auto index=mIndices[3*e.id+j];if(index>=mVcount)return {};for(unsigned k=0;k<3;++k)source.geometry.vertices[j][k]=mVertices[3*index+k];}bool local=true;for(unsigned k=0;k<3;++k){float lo=source.geometry.vertices[0][k],hi=lo;for(unsigned j=1;j<3;++j){lo=std::min(lo,source.geometry.vertices[j][k]);hi=std::max(hi,source.geometry.vertices[j][k]);}if(double(hi)<double(bounds.first[k])-limits.search_margin||double(lo)>double(bounds.second[k])+limits.search_margin)local=false;}if(local){if(sources.size()>=limits.component)return {};sources.push_back(source);}}
   // Bounded uncached candidate: no planar component sharing across curved seeds.
   return Housing::CurvedMount::Select(sources,seed,Housing::CurvedMount::Mode::Wall,volume,limits);
  }catch(...){return {};}
 }
 std::vector<RmUint32> sourcePlanarMountPatch(const Housing::OrientedMount::Volume&volume,const Housing::PlanarMount::SurfaceSeed&seed,const std::vector<RmUint32>&candidates) const override {
  try{if(!mRoot||!Housing::OrientedMount::Valid(volume)||(seed.side!=1&&seed.side!=-1)||seed.triangle_id>=mTcount||candidates.empty()||!std::is_sorted(candidates.begin(),candidates.end())||std::adjacent_find(candidates.begin(),candidates.end())!=candidates.end()||candidates.back()>=mTcount||!std::binary_search(candidates.begin(),candidates.end(),seed.triangle_id))return {};
   std::shared_ptr<const MountTopology>prepared;
   {std::lock_guard<std::mutex>lock(mMountMutex);auto&seeds=mPlanarTopology[candidates];auto prior=seeds.find(seed.triangle_id);if(prior!=seeds.end())prepared=prior->second;else{
    std::vector<Housing::MountSurfacePatch::Triangle>source;source.reserve(candidates.size());for(auto id:candidates){Housing::MountSurfacePatch::Triangle triangle;triangle.id=id;for(unsigned j=0;j<3;++j){const auto index=mIndices[3*id+j];if(index>=mVcount)return {};for(unsigned k=0;k<3;++k)triangle.vertices[j][k]=mVertices[3*index+k];}source.push_back(triangle);}
    auto entry=std::make_shared<MountTopology>();entry->ids=Housing::PlanarMount::Connected(source,seed.triangle_id);for(const auto&triangle:source)if(std::binary_search(entry->ids.begin(),entry->ids.end(),triangle.id))entry->triangles.push_back(triangle);prepared=entry;for(auto id:entry->ids)seeds.emplace(id,entry);
   }}
   return prepared&&!prepared->ids.empty()&&Housing::PlanarMount::Covered(prepared->triangles,prepared->ids,seed.triangle_id,volume)?prepared->ids:std::vector<RmUint32>{};
  }catch(...){return {};}
 }
 bool raycastSurface(const RmReal *from,const RmReal *to,RmReal *hitLocation,RmReal *hitNormal,RmReal *hitDistance,Housing::PlanarMount::SurfaceSeed&seed) override {
  seed={};try{if(!mRoot)return false;for(unsigned k=0;k<3;++k)if(!std::isfinite(from[k])||!std::isfinite(to[k]))return false;RmReal dir[3];for(unsigned k=0;k<3;++k)dir[k]=to[k]-from[k];RmReal distance=sqrtf(dir[0]*dir[0]+dir[1]*dir[1]+dir[2]*dir[2]);if(!std::isfinite(distance)||distance<.0000000001f)return false;const RmReal inverse=1.f/distance;for(auto&v:dir)v*=inverse;mRaycastFrame++;RmUint32 nearest=TRI_EOF;bool hit=false;
   mRoot->raycast(hit,from,to,dir,hitLocation,hitNormal,hitDistance,mVertices,mIndices,distance,this,mRaycastTriangles,mRaycastFrame,mLeafTriangles,nearest);if(!hit||nearest>=mTcount)return false;
   Housing::MountSurfacePatch::Triangle triangle;triangle.id=nearest;for(unsigned j=0;j<3;++j){const auto index=mIndices[3*nearest+j];if(index>=mVcount)return false;for(unsigned k=0;k<3;++k)triangle.vertices[j][k]=mVertices[3*index+k];}const auto frame=Housing::PlanarMount::Basis(triangle);Housing::PlanarMount::Vector origin;for(unsigned k=0;k<3;++k)origin[k]=Housing::OrientedMount::Exact(double(from[k]));const auto facing=Housing::OrientedMount::Dot(frame.normal,Housing::PlanarMount::Minus(origin,frame.origin));if(facing==0)return false;seed={nearest,facing>0?1:-1};return true;
  }catch(...){seed={};return false;}
 }

	MyRaycastMesh(RmUint32 vcount,const RmReal *vertices,RmUint32 tcount,const RmUint32 *indices,RmUint32 maxDepth,RmUint32 minLeafSize,RmReal minAxisSize)
	{
		mRaycastFrame = 0;
		if ( maxDepth < 2 )
		{
			maxDepth = 2;
		}
		if ( maxDepth > 15 )
		{
			maxDepth = 15;
		}
		RmUint32 pow2Table[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 65536 };
		mMaxNodeCount = 0;
		for (RmUint32 i=0; i<=maxDepth; i++)
		{
			mMaxNodeCount+=pow2Table[i];
		}
		// Allocate page-aligned memory
		mNodes = static_cast<NodeAABB*>(KSM::AllocatePageAligned(sizeof(NodeAABB) * mMaxNodeCount));
		if (!mNodes) {
			throw std::bad_alloc();
		}
		mNodeCount = 0;
		KSM::CheckPageAlignment(mNodes);

		mVertices = static_cast<RmReal*>(KSM::AllocatePageAligned(sizeof(RmReal) * 3 * vcount));
		if (!mVertices) {
			throw std::bad_alloc();
		}
		std::memcpy(mVertices, vertices, sizeof(RmReal) * 3 * vcount);
		mVcount = vcount;

		mIndices = static_cast<RmUint32*>(KSM::AllocatePageAligned(sizeof(RmUint32) * 3 * tcount));
		if (!mIndices) {
			throw std::bad_alloc();
		}
		std::memcpy(mIndices, indices, sizeof(RmUint32) * 3 * tcount);
		mTcount = tcount;

		mRaycastTriangles = static_cast<RmUint32*>(KSM::AllocatePageAligned(sizeof(RmUint32) * tcount));
		if (!mRaycastTriangles) {
			throw std::bad_alloc();
		}
		std::memset(mRaycastTriangles, 0, sizeof(RmUint32) * tcount);

		mFaceNormals = static_cast<RmReal*>(KSM::AllocatePageAligned(sizeof(RmReal) * 3 * tcount));
		if (!mFaceNormals) {
			throw std::bad_alloc();
		}
		std::memset(mFaceNormals, 0, sizeof(RmReal) * 3 * tcount);

		// Mark memory as mergeable for KSM
		KSM::MarkMemoryForKSM(mVertices, sizeof(RmReal) * 3 * vcount);
		KSM::MarkMemoryForKSM(mIndices, sizeof(RmUint32) * 3 * tcount);
		KSM::MarkMemoryForKSM(mRaycastTriangles, sizeof(RmUint32) * tcount);
		KSM::MarkMemoryForKSM(mFaceNormals, sizeof(RmReal) * 3 * tcount);

		mRoot = getNode();
		mFaceNormals = NULL;
		new ( mRoot ) NodeAABB(mVcount,mVertices,mTcount,mIndices,maxDepth,minLeafSize,minAxisSize,this,mLeafTriangles);

		KSM::MarkMemoryForKSM(mLeafTriangles.data(), mLeafTriangles.size() * sizeof(RmUint32));
	}

	~MyRaycastMesh(void)
	{
		if (mNodes) { free(mNodes); }
		if (mVertices) { free(mVertices); }
		if (mIndices) { free(mIndices); }
		if (mRaycastTriangles) { free(mRaycastTriangles); }
		if (mFaceNormals) { free(mFaceNormals); }
	}


	bool raycastExceptSourceRange(const RmReal *from, const RmReal *to, RmUint32 begin, RmUint32 end) override
	{
		if (!mRoot || begin >= end || end > mTcount) {
			return raycast(from, to, nullptr, nullptr, nullptr);
		}
		RmReal dir[3];
		for (unsigned k = 0; k < 3; ++k) {
			if (!std::isfinite(from[k]) || !std::isfinite(to[k])) {
				return true;
			}
			dir[k] = to[k] - from[k];
		}
		RmReal distance = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
		if (!std::isfinite(distance)) {
			return true;
		}
		if (distance < .0000000001f) {
			return false;
		}
		const RmReal inverse = 1.0f / distance;
		for (auto &v : dir) {
			v *= inverse;
		}
		++mRaycastFrame;
		RmUint32 nearest = TRI_EOF;
		bool hit = false;
		mRoot->raycast(hit, from, to, dir, nullptr, nullptr, nullptr, mVertices, mIndices, distance, this, mRaycastTriangles, mRaycastFrame, mLeafTriangles, nearest, begin, end);
		return hit;
	}
	virtual bool raycast(const RmReal *from,const RmReal *to,RmReal *hitLocation,RmReal *hitNormal,RmReal *hitDistance)
	{
		bool ret = false;

		RmReal dir[3];
		dir[0] = to[0] - from[0];
		dir[1] = to[1] - from[1];
		dir[2] = to[2] - from[2];
		RmReal distance = sqrtf( dir[0]*dir[0] + dir[1]*dir[1]+dir[2]*dir[2] );
		if ( distance < 0.0000000001f ) return false;
		RmReal recipDistance = 1.0f / distance;
		dir[0]*=recipDistance;
		dir[1]*=recipDistance;
		dir[2]*=recipDistance;
		mRaycastFrame++;
		RmUint32 nearestTriIndex=TRI_EOF;
		mRoot->raycast(ret,from,to,dir,hitLocation,hitNormal,hitDistance,mVertices,mIndices,distance,this,mRaycastTriangles,mRaycastFrame,mLeafTriangles,nearestTriIndex);
		return ret;
	}

	virtual void release(void)
	{
		delete this;
	}

	virtual const RmReal * getBoundMin(void) const // return the minimum bounding box
	{
		return mRoot->mBounds.mMin;
	}
	virtual const RmReal * getBoundMax(void) const // return the maximum bounding box.
	{
		return mRoot->mBounds.mMax;
	}

	virtual NodeAABB * getNode(void)
	{
		assert( mNodeCount < mMaxNodeCount );
		NodeAABB *ret = &mNodes[mNodeCount];
		mNodeCount++;
		return ret;
	}

	virtual void getFaceNormal(RmUint32 tri,RmReal *faceNormal)
	{
		if ( mFaceNormals == NULL )
		{
			mFaceNormals = (RmReal *)::malloc(sizeof(RmReal)*3*mTcount);
			for (RmUint32 i=0; i<mTcount; i++)
			{
				RmUint32 i1		= mIndices[i*3+0];
				RmUint32 i2		= mIndices[i*3+1];
				RmUint32 i3		= mIndices[i*3+2];
				const RmReal*p1 = &mVertices[i1*3];
				const RmReal*p2 = &mVertices[i2*3];
				const RmReal*p3 = &mVertices[i3*3];
				RmReal *dest	= &mFaceNormals[i*3];
				computePlane(p3,p2,p1,dest);
			}
		}
		const RmReal *src = &mFaceNormals[tri*3];
		faceNormal[0] = src[0];
		faceNormal[1] = src[1];
		faceNormal[2] = src[2];
	}

	virtual bool bruteForceRaycast(const RmReal *from,const RmReal *to,RmReal *hitLocation,RmReal *hitNormal,RmReal *hitDistance)
	{
		bool ret = false;

		RmReal dir[3];

		dir[0] = to[0] - from[0];
		dir[1] = to[1] - from[1];
		dir[2] = to[2] - from[2];

		RmReal distance = sqrtf( dir[0]*dir[0] + dir[1]*dir[1]+dir[2]*dir[2] );
		if ( distance < 0.0000000001f ) return false;
		RmReal recipDistance = 1.0f / distance;
		dir[0]*=recipDistance;
		dir[1]*=recipDistance;
		dir[2]*=recipDistance;
		const RmUint32 *indices = mIndices;
		const RmReal *vertices = mVertices;
		RmReal nearestDistance = distance;

		for (RmUint32 tri=0; tri<mTcount; tri++)
		{
			RmUint32 i1 = indices[tri*3+0];
			RmUint32 i2 = indices[tri*3+1];
			RmUint32 i3 = indices[tri*3+2];

			const RmReal *p1 = &vertices[i1*3];
			const RmReal *p2 = &vertices[i2*3];
			const RmReal *p3 = &vertices[i3*3];

			RmReal t;
			if ( rayIntersectsTriangle(from,dir,p1,p2,p3,t))
			{
				if ( t < nearestDistance )
				{
					nearestDistance = t;
					if ( hitLocation )
					{
						hitLocation[0] = from[0]+dir[0]*t;
						hitLocation[1] = from[1]+dir[1]*t;
						hitLocation[2] = from[2]+dir[2]*t;
					}

					if ( hitNormal )
					{
						getFaceNormal(tri,hitNormal);
					}

					if ( hitDistance )
					{
						*hitDistance = t;
					}
					ret = true;
				}
			}
		}
		return ret;
	}

	RmUint32		mRaycastFrame;
	RmUint32		*mRaycastTriangles;
	RmUint32		mVcount;
	RmReal			*mVertices;
	RmReal			*mFaceNormals;
	RmUint32		mTcount;
	RmUint32		*mIndices;
	NodeAABB		*mRoot;
	RmUint32		mNodeCount;
	RmUint32		mMaxNodeCount;
	NodeAABB		*mNodes;
	TriVector		mLeafTriangles;

#ifdef USE_MAP_MMFS
	MyRaycastMesh(std::vector<char>& rm_buffer);
	void serialize(std::vector<char>& rm_buffer);
#endif /*USE_MAP_MMFS*/
};

};



using namespace RAYCAST_MESH;


RaycastMesh * createRaycastMesh(RmUint32 vcount,		// The number of vertices in the source triangle mesh
								const RmReal *vertices,		// The array of vertex positions in the format x1,y1,z1..x2,y2,z2.. etc.
								RmUint32 tcount,		// The number of triangles in the source triangle mesh
								const RmUint32 *indices, // The triangle indices in the format of i1,i2,i3 ... i4,i5,i6, ...
								RmUint32 maxDepth,	// Maximum recursion depth for the triangle mesh.
								RmUint32 minLeafSize,	// minimum triangles to treat as a 'leaf' node.
								RmReal	minAxisSize	// once a particular axis is less than this size, stop sub-dividing.
								)
{
	auto m = new MyRaycastMesh(vcount, vertices, tcount, indices, maxDepth, minLeafSize, minAxisSize);

	// Calculate memory usage
	size_t vertex_size = vcount * sizeof(RmReal) * 3;     // Each vertex has 3 floats
	size_t index_size = tcount * 3 * sizeof(RmUint32);    // Each triangle has 3 indices
	size_t bvh_node_size = m->mNodeCount * sizeof(NodeAABB); // BVH Node memory usage
	size_t bvh_leaf_size = m->mLeafTriangles.size() * sizeof(RmUint32); // BVH leaf triangles

	size_t bvh_size = bvh_node_size + bvh_leaf_size; // Total BVH size
	size_t total_size = vertex_size + index_size + bvh_size;

	KSM::CheckPageAlignment(m->mNodes);
	KSM::CheckPageAlignment(m->mVertices);

	LogInfo(
		"Map Raycast Memory Usage | Vertices [{:.2f}] MB Indices [{:.2f}] MB BVH Nodes [{:.2f}] MB BVH Leaves [{:.2f}] MB BVH Total [{:.2f}] MB",
		vertex_size / (1024.0 * 1024.0),
		index_size / (1024.0 * 1024.0),
		bvh_node_size / (1024.0 * 1024.0),
		bvh_leaf_size / (1024.0 * 1024.0),
		bvh_size / (1024.0 * 1024.0)
	);
	LogInfo("Total Raycast Memory [{:.2f}] MB", total_size / (1024.0 * 1024.0));

	return static_cast< RaycastMesh * >(m);
}

#ifdef USE_MAP_MMFS
RaycastMesh* loadRaycastMesh(std::vector<char>& rm_buffer, bool& load_success)
{
	if (rm_buffer.empty())
		return nullptr;

	auto m = new MyRaycastMesh(rm_buffer);
	load_success = (m->mNodes != nullptr);

	return static_cast<RaycastMesh*>(m);
}

void serializeRaycastMesh(RaycastMesh* rm, std::vector<char>& rm_buffer)
{
	if (!rm) {
		rm_buffer.clear();
		return;
	}

	static_cast<MyRaycastMesh*>(rm)->serialize(rm_buffer);
}

MyRaycastMesh::MyRaycastMesh(std::vector<char>& rm_buffer)
{
	mVcount = 0;
	mVertices = nullptr;
	mTcount = 0;
	mIndices = nullptr;
	mRaycastTriangles = nullptr;
	mFaceNormals = nullptr;
	mRaycastFrame = 0;
	mMaxNodeCount = 0;
	mNodeCount = 0;
	mNodes = nullptr;
	mRoot = nullptr;

	size_t chunk_size = 0;
	size_t bytes_read = 0;
	size_t rm_buffer_size_ = rm_buffer.size();
	if (!rm_buffer_size_)
		return;

	char* buf = rm_buffer.data();

	chunk_size = sizeof(RmUint32);
	memcpy(&mVcount, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	chunk_size = (sizeof(RmReal) * (3 * mVcount));
	mVertices = (RmReal *)::malloc(chunk_size);
	memcpy(mVertices, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	chunk_size = sizeof(RmUint32);
	memcpy(&mTcount, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	chunk_size = (sizeof(RmUint32) * (3 * mTcount));
	mIndices = (RmUint32 *)::malloc(chunk_size);
	memcpy(mIndices, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	chunk_size = (sizeof(RmUint32) * mTcount);
	mRaycastTriangles = (RmUint32 *)::malloc(chunk_size);
	memcpy(mRaycastTriangles, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	chunk_size = (sizeof(RmReal) * (3 * mTcount));
	mFaceNormals = (RmReal *)::malloc(chunk_size);
	memcpy(mFaceNormals, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	chunk_size = sizeof(RmUint32);
	memcpy(&mRaycastFrame, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	RmUint32 lt_size;
	chunk_size = sizeof(RmUint32);
	memcpy(&lt_size, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	if (lt_size) {
		mLeafTriangles.resize(lt_size);
		chunk_size = (sizeof(RmUint32) * lt_size);
		memcpy(&mLeafTriangles[0], buf, chunk_size);
		buf += chunk_size;
		bytes_read += chunk_size;
	}

	chunk_size = sizeof(RmUint32);
	memcpy(&mNodeCount, buf, chunk_size);
	buf += chunk_size;
	bytes_read += chunk_size;

	mMaxNodeCount = mNodeCount;

	mNodes = new NodeAABB[mMaxNodeCount];
	mRoot = &mNodes[0];

	for (RmUint32 index = 0; index < mNodeCount; ++index) {
		chunk_size = (sizeof(RmReal) * 3);
		memcpy(&mNodes[index].mBounds.mMin, buf, chunk_size);
		buf += chunk_size;
		bytes_read += chunk_size;

		chunk_size = (sizeof(RmReal) * 3);
		memcpy(&mNodes[index].mBounds.mMax, buf, chunk_size);
		buf += chunk_size;
		bytes_read += chunk_size;

		chunk_size = sizeof(RmUint32);
		memcpy(&mNodes[index].mLeafTriangleIndex, buf, chunk_size);
		buf += chunk_size;
		bytes_read += chunk_size;

		RmUint32 lNodeIndex;
		chunk_size = sizeof(RmUint32);
		memcpy(&lNodeIndex, buf, chunk_size);
		if (lNodeIndex != TRI_EOF)
			mNodes[index].mLeft = &mNodes[lNodeIndex];
		buf += chunk_size;
		bytes_read += chunk_size;

		RmUint32 rNodeIndex;
		chunk_size = sizeof(RmUint32);
		memcpy(&rNodeIndex, buf, chunk_size);
		if (rNodeIndex != TRI_EOF)
			mNodes[index].mRight = &mNodes[rNodeIndex];
		buf += chunk_size;
		bytes_read += chunk_size;
	}

	if (bytes_read != rm_buffer_size_) {
		delete[] mNodes;
		::free(mVertices);
		::free(mIndices);
		::free(mFaceNormals);
		::free(mRaycastTriangles);

		mVcount = 0;
		mVertices = nullptr;
		mTcount = 0;
		mIndices = nullptr;
		mRaycastTriangles = nullptr;
		mFaceNormals = nullptr;
		mRaycastFrame = 0;
		mLeafTriangles.clear();
		mMaxNodeCount = 0;
		mNodeCount = 0;
		mNodes = nullptr;
		mRoot = nullptr;
	}
}

void MyRaycastMesh::serialize(std::vector<char>& rm_buffer)
{
	rm_buffer.clear();

	size_t rm_buffer_size_ = 0;

	rm_buffer_size_ += sizeof(RmUint32); // mVcount
	rm_buffer_size_ += (sizeof(RmReal) * (3 * mVcount)); // mVertices
	rm_buffer_size_ += sizeof(RmUint32); // mTcount
	rm_buffer_size_ += (sizeof(RmUint32) * (3 * mTcount)); // mIndices
	rm_buffer_size_ += (sizeof(RmUint32) * mTcount); // mRaycastTriangles
	rm_buffer_size_ += (sizeof(RmReal) * (3 * mTcount)); // mFaceNormals
	rm_buffer_size_ += sizeof(RmUint32); // mRaycastFrame
	rm_buffer_size_ += sizeof(RmUint32); // mLeafTriangles.size()
	rm_buffer_size_ += (sizeof(RmUint32) * (RmUint32)mLeafTriangles.size()); // mLeafTriangles
	rm_buffer_size_ += sizeof(RmUint32); // mNodeCount
	rm_buffer_size_ += (sizeof(RmReal) * (3 * mNodeCount)); // mNodes.mBounds.mMin
	rm_buffer_size_ += (sizeof(RmReal) * (3 * mNodeCount)); // mNodes.mBounds.mMax
	rm_buffer_size_ += (sizeof(RmUint32) * mNodeCount); // mNodes.mLeafTriangleIndex
	rm_buffer_size_ += (sizeof(RmUint32) * mNodeCount); // mNodes.mLeft[Index]
	rm_buffer_size_ += (sizeof(RmUint32) * mNodeCount); // mNodes.mRight[Index]

	rm_buffer.resize(rm_buffer_size_);

	char* buf = rm_buffer.data();

	memcpy(buf, &mVcount, sizeof(RmUint32));
	buf += sizeof(RmUint32);

	memcpy(buf, mVertices, (sizeof(RmReal) * (3 * mVcount)));
	buf += (sizeof(RmReal) * (3 * mVcount));

	memcpy(buf, &mTcount, sizeof(RmUint32));
	buf += sizeof(RmUint32);

	memcpy(buf, mIndices, (sizeof(RmUint32) * (3 * mTcount)));
	buf += (sizeof(RmUint32) * (3 * mTcount));

	memcpy(buf, mRaycastTriangles, (sizeof(RmUint32) * mTcount));
	buf += (sizeof(RmUint32) * mTcount);

	if (!mFaceNormals) {
		RmReal save_face[3];
		getFaceNormal(0, &save_face[0]);
	}

	memcpy(buf, mFaceNormals, (sizeof(RmReal) * (3 * mTcount)));
	buf += (sizeof(RmReal) * (3 * mTcount));

	memcpy(buf, &mRaycastFrame, sizeof(RmUint32));
	buf += sizeof(RmUint32);

	RmUint32 lt_size = (RmUint32)mLeafTriangles.size();
	memcpy(buf, &lt_size, sizeof(RmUint32));
	buf += sizeof(RmUint32);

	if (lt_size) {
		memcpy(buf, &mLeafTriangles[0], (sizeof(RmUint32) * lt_size));
		buf += (sizeof(RmUint32) * lt_size);
	}

	memcpy(buf, &mNodeCount, sizeof(RmUint32));
	buf += sizeof(RmUint32);

	for (RmUint32 index = 0; index < mNodeCount; ++index) {
		memcpy(buf, &mNodes[index].mBounds.mMin, (sizeof(RmReal) * 3));
		buf += (sizeof(RmReal) * 3);

		memcpy(buf, &mNodes[index].mBounds.mMax, (sizeof(RmReal) * 3));
		buf += (sizeof(RmReal) * 3);

		memcpy(buf, &mNodes[index].mLeafTriangleIndex, sizeof(RmUint32));
		buf += sizeof(RmUint32);

		RmUint32 lNodeIndex = TRI_EOF;
		if (mNodes[index].mLeft)
			lNodeIndex = ((uintptr_t)mNodes[index].mLeft - (uintptr_t)mNodes) / sizeof(NodeAABB);
		memcpy(buf, &lNodeIndex, sizeof(RmUint32));
		buf += sizeof(RmUint32);

		RmUint32 rNodeIndex = TRI_EOF;
		if (mNodes[index].mRight)
			rNodeIndex = ((uintptr_t)mNodes[index].mRight - (uintptr_t)mNodes) / sizeof(NodeAABB);
		memcpy(buf, &rNodeIndex, sizeof(RmUint32));
		buf += sizeof(RmUint32);
	}
}
#endif /*USE_MAP_MMFS*/
