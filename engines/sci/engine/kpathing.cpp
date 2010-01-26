/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "sci/sci.h"
#include "sci/engine/state.h"
#include "sci/engine/kernel.h"
#include "sci/graphics/gui.h"

#include "common/list.h"

namespace Sci {

#define AVOIDPATH_DYNMEM_STRING "AvoidPath polyline"

#define POLY_LAST_POINT 0x7777
#define POLY_POINT_SIZE 4
//#define DEBUG_AVOIDPATH	//enable for avoidpath debugging

// SCI-defined polygon types
enum {
	POLY_TOTAL_ACCESS = 0,
	POLY_NEAREST_ACCESS = 1,
	POLY_BARRED_ACCESS = 2,
	POLY_CONTAINED_ACCESS = 3
};

// Polygon containment types
enum {
	CONT_OUTSIDE = 0,
	CONT_ON_EDGE = 1,
	CONT_INSIDE = 2
};

#define HUGE_DISTANCE 0xFFFFFFFF

#define VERTEX_HAS_EDGES(V) ((V) != CLIST_NEXT(V))

// Error codes
enum {
	PF_OK = 0,
	PF_ERROR = -1,
	PF_FATAL = -2
};

// Floating point struct
struct FloatPoint {
	FloatPoint() : x(0), y(0) {}
	FloatPoint(float x_, float y_) : x(x_), y(y_) {}

	Common::Point toPoint() {
		return Common::Point((int16)(x + 0.5), (int16)(y + 0.5));
	}

	float x, y;
};

struct Vertex {
	// Location
	Common::Point v;

	// Vertex circular list entry
	Vertex *_next;	// next element
	Vertex *_prev;	// previous element

	// A* cost variables
	uint32 costF;
	uint32 costG;

	// Previous vertex in shortest path
	Vertex *path_prev;

public:
	Vertex(const Common::Point &p) : v(p) {
		costG = HUGE_DISTANCE;
		path_prev = NULL;
	}
};

class VertexList: public Common::List<Vertex *> {
public:
	bool contains(Vertex *v) {
		for (iterator it = begin(); it != end(); ++it) {
			if (v == *it)
				return true;
		}
		return false;
	}
};

/* Circular list definitions. */

#define CLIST_FOREACH(var, head)					\
	for ((var) = (head)->first();					\
		(var);							\
		(var) = ((var)->_next == (head)->first() ?	\
		    NULL : (var)->_next))

/* Circular list access methods. */
#define CLIST_NEXT(elm)		((elm)->_next)
#define CLIST_PREV(elm)		((elm)->_prev)

class CircularVertexList {
public:
	Vertex *_head;

public:
	CircularVertexList() : _head(0) {}

	Vertex *first() const {
		return _head;
	}

	void insertHead(Vertex *elm) {
		if (_head == NULL) {
			elm->_next = elm->_prev = elm;
		} else {
			elm->_next = _head;
			elm->_prev = _head->_prev;
			_head->_prev = elm;
			elm->_prev->_next = elm;
		}
		_head = elm;
	}

	static void insertAfter(Vertex *listelm, Vertex *elm) {
		elm->_prev = listelm;
		elm->_next = listelm->_next;
		listelm->_next->_prev = elm;
		listelm->_next = elm;
	}

	void remove(Vertex *elm) {
		if (elm->_next == elm) {
			_head = NULL;
		} else {
			if (_head == elm)
				_head = elm->_next;
			elm->_prev->_next = elm->_next;
			elm->_next->_prev = elm->_prev;
		}
	}

	bool empty() const {
		return _head == NULL;
	}

	uint size() const {
		int n = 0;
		Vertex *v;
		CLIST_FOREACH(v, this)
			++n;
		return n;
	}

	/**
	 * Reverse the order of the elements in this circular list.
	 */
	void reverse() {
		if (!_head)
			return;

		Vertex *elm = _head;
		do {
			SWAP(elm->_prev, elm->_next);
			elm = elm->_next;
		} while (elm != _head);
	}
};

struct Polygon {
	// SCI polygon type
	int type;

	// Circular list of vertices
	CircularVertexList vertices;

public:
	Polygon(int t) : type(t) {
	}

	~Polygon() {
		while (!vertices.empty()) {
			Vertex *vertex = vertices.first();
			vertices.remove(vertex);
			delete vertex;
		}
	}
};

typedef Common::List<Polygon *> PolygonList;

// Pathfinding state
struct PathfindingState {
	// List of all polygons
	PolygonList polygons;

	// Start and end points for pathfinding
	Vertex *vertex_start, *vertex_end;

	// Array of all vertices, used for sorting
	Vertex **vertex_index;

	// Total number of vertices
	int vertices;

	// Point to prepend and append to final path
	Common::Point *_prependPoint;
	Common::Point *_appendPoint;

	// Screen size
	int _width, _height;

	PathfindingState(int width, int height) : _width(width), _height(height) {
		vertex_start = NULL;
		vertex_end = NULL;
		vertex_index = NULL;
		_prependPoint = NULL;
		_appendPoint = NULL;
		vertices = 0;
	}

	~PathfindingState() {
		free(vertex_index);

		delete _prependPoint;
		delete _appendPoint;

		for (PolygonList::iterator it = polygons.begin(); it != polygons.end(); ++it) {
			delete *it;
		}
	}

	bool pointOnScreenBorder(const Common::Point &p);
	bool edgeOnScreenBorder(const Common::Point &p, const Common::Point &q);
	int findNearPoint(const Common::Point &p, Polygon *polygon, Common::Point *ret);
};


static Common::Point read_point(SegManager *segMan, reg_t list, int offset) {
	SegmentRef list_r = segMan->dereference(list);
	if (!list_r.isValid() || list_r.skipByte) {
		warning("read_point(): Attempt to dereference invalid pointer %04x:%04x", PRINT_REG(list));
	}
	Common::Point point;

	if (list_r.isRaw) {
		point.x = (int16)READ_LE_UINT16(list_r.raw + offset * POLY_POINT_SIZE);
		point.y = (int16)READ_LE_UINT16(list_r.raw + offset * POLY_POINT_SIZE + 2);
	} else {
		point.x = list_r.reg[offset * 2].toUint16();
		point.y = list_r.reg[offset * 2 + 1].toUint16();
	}
	return point;
}

static void writePoint(SegmentRef ref, int offset, const Common::Point &point) {
	if (ref.isRaw) {
		WRITE_LE_UINT16(ref.raw + offset * POLY_POINT_SIZE, point.x);
		WRITE_LE_UINT16(ref.raw + offset * POLY_POINT_SIZE + 2, point.y);
	} else {
		ref.reg[offset * 2] = make_reg(0, point.x);
		ref.reg[offset * 2 + 1] = make_reg(0, point.y);
	}
}

#ifdef DEBUG_AVOIDPATH

static void draw_line(EngineState *s, Common::Point p1, Common::Point p2, int type, int width, int height) {
	// Colors for polygon debugging.
	// Green: Total access
	// Blue: Near-point access
	// Red : Barred access
	// Yellow: Contained access
	int poly_colors[4] = {
		s->_gui->paletteFind(0, 255, 0),	// green
		s->_gui->paletteFind(0, 0, 255),	// blue
		s->_gui->paletteFind(255, 0, 0),	// red
		s->_gui->paletteFind(255, 255, 0)	// yellow
	};

	// Clip
	// FIXME: Do proper line clipping
	p1.x = CLIP<int16>(p1.x, 0, width - 1);
	p1.y = CLIP<int16>(p1.y, 0, height - 1);
	p2.x = CLIP<int16>(p2.x, 0, width - 1);
	p2.y = CLIP<int16>(p2.y, 0, height - 1);

	assert(type >= 0 && type <= 3);
	s->_gui->graphDrawLine(p1, p2, poly_colors[type], 255, 255);
}

static void draw_point(EngineState *s, Common::Point p, int start, int width, int height) {
	// Colors for starting and end point
	// Green: End point
	// Blue: Starting point
	int point_colors[2] = {
		s->_gui->paletteFind(0, 255, 0),	// green
		s->_gui->paletteFind(0, 0, 255)		// blue
	};

	Common::Rect rect = Common::Rect(p.x - 1, p.y - 1, p.x - 1 + 3, p.y - 1 + 3);

	// Clip
	rect.top = CLIP<int16>(rect.top, 0, height - 1);
	rect.bottom = CLIP<int16>(rect.bottom, 0, height - 1);
	rect.left = CLIP<int16>(rect.left, 0, width - 1);
	rect.right = CLIP<int16>(rect.right, 0, width - 1);

	assert(start >= 0 && start <= 1);
	s->_gui->graphFrameBox(rect, point_colors[start]);
}

static void draw_polygon(EngineState *s, reg_t polygon, int width, int height) {
	SegManager *segMan = s->_segMan;
	reg_t points = GET_SEL32(segMan, polygon, points);

#ifdef ENABLE_SCI32
	if (segMan->isHeapObject(points))
		points = GET_SEL32(segMan, points, data);
#endif

	int size = GET_SEL32(segMan, polygon, size).toUint16();
	int type = GET_SEL32(segMan, polygon, type).toUint16();
	Common::Point first, prev;
	int i;

	prev = first = read_point(segMan, points, 0);

	for (i = 1; i < size; i++) {
		Common::Point point = read_point(segMan, points, i);
		draw_line(s, prev, point, type, width, height);
		prev = point;
	}

	draw_line(s, prev, first, type % 3, width, height);
}

static void draw_input(EngineState *s, reg_t poly_list, Common::Point start, Common::Point end, int opt, int width, int height) {
	List *list;
	Node *node;

	draw_point(s, start, 1, width, height);
	draw_point(s, end, 0, width, height);

	if (!poly_list.segment)
		return;

	list = s->_segMan->lookupList(poly_list);

	if (!list) {
		warning("[avoidpath] Could not obtain polygon list");
		return;
	}

	node = s->_segMan->lookupNode(list->first);

	while (node) {
		draw_polygon(s, node->value, width, height);
		node = s->_segMan->lookupNode(node->succ);
	}
}

#endif	// DEBUG_AVOIDPATH

static void print_polygon(SegManager *segMan, reg_t polygon) {
	reg_t points = GET_SEL32(segMan, polygon, points);

#ifdef ENABLE_SCI32
	if (segMan->isHeapObject(points))
		points = GET_SEL32(segMan, points, data);
#endif

	int size = GET_SEL32(segMan, polygon, size).toUint16();
	int type = GET_SEL32(segMan, polygon, type).toUint16();
	int i;
	Common::Point point;

	printf("%i:", type);

	for (i = 0; i < size; i++) {
		point = read_point(segMan, points, i);
		printf(" (%i, %i)", point.x, point.y);
	}

	point = read_point(segMan, points, 0);
	printf(" (%i, %i);\n", point.x, point.y);
}

static void print_input(EngineState *s, reg_t poly_list, Common::Point start, Common::Point end, int opt) {
	List *list;
	Node *node;

	printf("Start point: (%i, %i)\n", start.x, start.y);
	printf("End point: (%i, %i)\n", end.x, end.y);
	printf("Optimization level: %i\n", opt);

	if (!poly_list.segment)
		return;

	list = s->_segMan->lookupList(poly_list);

	if (!list) {
		warning("[avoidpath] Could not obtain polygon list");
		return;
	}

	printf("Polygons:\n");
	node = s->_segMan->lookupNode(list->first);

	while (node) {
		print_polygon(s->_segMan, node->value);
		node = s->_segMan->lookupNode(node->succ);
	}
}

static int area(const Common::Point &a, const Common::Point &b, const Common::Point &c) {
	// Computes the area of a triangle
	// Parameters: (const Common::Point &) a, b, c: The points of the triangle
	// Returns   : (int) The area multiplied by two
	return (b.x - a.x) * (a.y - c.y) - (c.x - a.x) * (a.y - b.y);
}

static bool left(const Common::Point &a, const Common::Point &b, const Common::Point &c) {
	// Determines whether or not a point is to the left of a directed line
	// Parameters: (const Common::Point &) a, b: The directed line (a, b)
	//             (const Common::Point &) c: The query point
	// Returns   : (int) true if c is to the left of (a, b), false otherwise
	return area(a, b, c) > 0;
}

static bool collinear(const Common::Point &a, const Common::Point &b, const Common::Point &c) {
	// Determines whether or not three points are collinear
	// Parameters: (const Common::Point &) a, b, c: The three points
	// Returns   : (int) true if a, b, and c are collinear, false otherwise
	return area(a, b, c) == 0;
}

static bool between(const Common::Point &a, const Common::Point &b, const Common::Point &c) {
	// Determines whether or not a point lies on a line segment
	// Parameters: (const Common::Point &) a, b: The line segment (a, b)
	//             (const Common::Point &) c: The query point
	// Returns   : (int) true if c lies on (a, b), false otherwise
	if (!collinear(a, b, c))
		return false;

	// Assumes a != b.
	if (a.x != b.x)
		return ((a.x <= c.x) && (c.x <= b.x)) || ((a.x >= c.x) && (c.x >= b.x));
	else
		return ((a.y <= c.y) && (c.y <= b.y)) || ((a.y >= c.y) && (c.y >= b.y));
}

static bool intersect_proper(const Common::Point &a, const Common::Point &b, const Common::Point &c, const Common::Point &d) {
	// Determines whether or not two line segments properly intersect
	// Parameters: (const Common::Point &) a, b: The line segment (a, b)
	//             (const Common::Point &) c, d: The line segment (c, d)
	// Returns   : (int) true if (a, b) properly intersects (c, d), false otherwise
	int ab = (left(a, b, c) && left(b, a, d)) || (left(a, b, d) && left(b, a, c));
	int cd = (left(c, d, a) && left(d, c, b)) || (left(c, d, b) && left(d, c, a));

	return ab && cd;
}

static int contained(const Common::Point &p, Polygon *polygon) {
	// Polygon containment test
	// Parameters: (const Common::Point &) p: The point
	//             (Polygon *) polygon: The polygon
	// Returns   : (int) CONT_INSIDE if p is strictly contained in polygon,
	//                   CONT_ON_EDGE if p lies on an edge of polygon,
	//                   CONT_OUTSIDE otherwise
	// Number of ray crossing left and right
	int lcross = 0, rcross = 0;
	Vertex *vertex;

	// Iterate over edges
	CLIST_FOREACH(vertex, &polygon->vertices) {
		const Common::Point &v1 = vertex->v;
		const Common::Point &v2 = CLIST_NEXT(vertex)->v;

		// Flags for ray straddling left and right
		int rstrad, lstrad;

		// Check if p is a vertex
		if (p == v1)
			return CONT_ON_EDGE;

		// Check if edge straddles the ray
		rstrad = (v1.y < p.y) != (v2.y < p.y);
		lstrad = (v1.y > p.y) != (v2.y > p.y);

		if (lstrad || rstrad) {
			// Compute intersection point x / xq
			int x = v2.x * v1.y - v1.x * v2.y + (v1.x - v2.x) * p.y;
			int xq = v1.y - v2.y;

			// Multiply by -1 if xq is negative (for comparison that follows)
			if (xq < 0) {
				x = -x;
				xq = -xq;
			}

			// Avoid floats by multiplying instead of dividing
			if (rstrad && (x > xq * p.x))
				rcross++;
			else if (lstrad && (x < xq * p.x))
				lcross++;
		}
	}

	// If we counted an odd number of total crossings the point is on an edge
	if ((lcross + rcross) % 2 == 1)
		return CONT_ON_EDGE;

	// If there are an odd number of crossings to one side the point is contained in the polygon
	if (rcross % 2 == 1) {
		// Invert result for contained access polygons.
		if (polygon->type == POLY_CONTAINED_ACCESS)
			return CONT_OUTSIDE;
		return CONT_INSIDE;
	}

	// Point is outside polygon. Invert result for contained access polygons
	if (polygon->type == POLY_CONTAINED_ACCESS)
		return CONT_INSIDE;

	return CONT_OUTSIDE;
}

static int polygon_area(Polygon *polygon) {
	// Computes polygon area
	// Parameters: (Polygon *) polygon: The polygon
	// Returns   : (int) The area multiplied by two
	Vertex *first = polygon->vertices.first();
	Vertex *v;
	int size = 0;

	v = CLIST_NEXT(first);

	while (CLIST_NEXT(v) != first) {
		size += area(first->v, v->v, CLIST_NEXT(v)->v);
		v = CLIST_NEXT(v);
	}

	return size;
}

static void fix_vertex_order(Polygon *polygon) {
	// Fixes the vertex order of a polygon if incorrect. Contained access
	// polygons should have their vertices ordered clockwise, all other types
	// anti-clockwise
	// Parameters: (Polygon *) polygon: The polygon
	int area = polygon_area(polygon);

	// When the polygon area is positive the vertices are ordered
	// anti-clockwise. When the area is negative the vertices are ordered
	// clockwise
	if (((area > 0) && (polygon->type == POLY_CONTAINED_ACCESS))
	        || ((area < 0) && (polygon->type != POLY_CONTAINED_ACCESS))) {

		polygon->vertices.reverse();
	}
}

static int inside(const Common::Point &p, Vertex *vertex) {
	// Determines whether or not a line from a point to a vertex intersects the
	// interior of the polygon, locally at that vertex
	// Parameters: (Common::Point) p: The point
	//             (Vertex *) vertex: The vertex
	// Returns   : (int) 1 if the line (p, vertex->v) intersects the interior of
	//                   the polygon, locally at the vertex. 0 otherwise
	// Check that it's not a single-vertex polygon
	if (VERTEX_HAS_EDGES(vertex)) {
		const Common::Point &prev = CLIST_PREV(vertex)->v;
		const Common::Point &next = CLIST_NEXT(vertex)->v;
		const Common::Point &cur = vertex->v;

		if (left(prev, cur, next)) {
			// Convex vertex, line (p, cur) intersects the inside
			// if p is located left of both edges
			if (left(cur, next, p) && left(prev, cur, p))
				return 1;
		} else {
			// Non-convex vertex, line (p, cur) intersects the
			// inside if p is located left of either edge
			if (left(cur, next, p) || left(prev, cur, p))
				return 1;
		}
	}

	return 0;
}

#ifdef OLD_PATHFINDING

/**
 * Checks whether two polygons are equal
 */
static bool polygons_equal(SegManager *segMan, reg_t p1, reg_t p2) {
	// Check for same type
	if (GET_SEL32(segMan, p1, type).toUint16() != GET_SEL32(segMan, p2, type).toUint16())
		return false;

	int size = GET_SEL32(segMan, p1, size).toUint16();

	// Check for same number of points
	if (size != GET_SEL32(segMan, p2, size).toUint16())
		return false;

	reg_t p1_points = GET_SEL32(segMan, p1, points);
	reg_t p2_points = GET_SEL32(segMan, p2, points);

	// Check for the same points
	for (int i = 0; i < size; i++) {
		if (read_point(segMan, p1_points, i) != read_point(segMan, p2_points, i))
			return false;
	}

	return true;
}

static bool left_on(const Common::Point &a, const Common::Point &b, const Common::Point &c) {
	// Determines whether or not a point is to the left of or collinear with a
	// directed line
	// Parameters: (const Common::Point &) a, b: The directed line (a, b)
	//             (const Common::Point &) c: The query point
	// Returns   : (int) true if c is to the left of or collinear with (a, b), false
	//                   otherwise
	return area(a, b, c) >= 0;
}

static Vertex *s_vertex_cur = 0;	// FIXME: Avoid non-const global vars

static int vertex_compare(const void *a, const void *b) {
	// Compares two vertices by angle (first) and distance (second) in relation
	// to s_vertex_cur. The angle is relative to the horizontal line extending
	// right from s_vertex_cur, and increases clockwise
	// Parameters: (const void *) a, b: The vertices
	// Returns   : (int) -1 if a is smaller than b, 1 if a is larger than b, and
	//                   0 if a and b are equal
	const Common::Point &p0 = s_vertex_cur->v;
	const Common::Point &p1 = (*(const Vertex **) a)->v;
	const Common::Point &p2 = (*(const Vertex **) b)->v;

	if (p1 == p2)
		return 0;

	// Points above p0 have larger angle than points below p0
	if ((p1.y < p0.y) && (p2.y >= p0.y))
		return 1;

	if ((p2.y < p0.y) && (p1.y >= p0.y))
		return -1;

	// Handle case where all points have the same y coordinate
	if ((p0.y == p1.y) && (p0.y == p2.y)) {
		// Points left of p0 have larger angle than points right of p0
		if ((p1.x < p0.x) && (p2.x >= p0.x))
			return 1;
		if ((p1.x >= p0.x) && (p2.x < p0.x))
			return -1;
	}

	if (collinear(p0, p1, p2)) {
		// At this point collinear points must have the same angle,
		// so compare distance to p0
		if (abs(p1.x - p0.x) < abs(p2.x - p0.x))
			return -1;
		if (abs(p1.y - p0.y) < abs(p2.y - p0.y))
			return -1;

		return 1;
	}

	// If p2 is left of the directed line (p0, p1) then p1 has greater angle
	if (left(p0, p1, p2))
		return 1;

	return -1;
}

static void clockwise(const Vertex *vertex_cur, const Vertex *v, const Common::Point *&p1, const Common::Point *&p2) {
	// Orders the points of an edge clockwise around vertex_cur. If all three
	// points are collinear the original order is used
	// Parameters: (const Vertex *) v: The first vertex of the edge
	// Returns   : ()
	//             (const Common::Point *&) p1: The first point in clockwise order
	//             (const Common::Point *&) p2: The second point in clockwise order
	Vertex *w = CLIST_NEXT(v);

	if (left_on(vertex_cur->v, w->v, v->v)) {
		p1 = &v->v;
		p2 = &w->v;
	} else {
		p1 = &w->v;
		p2 = &v->v;
	}
}

/**
 * Compares two edges that are intersected by the sweeping line by distance from vertex_cur
 * @param a				the first edge
 * @param b				the second edge
 * @return true if a is closer to vertex_cur than b, false otherwise
 */
static bool edgeIsCloser(const Vertex *vertex_cur, const Vertex *a, const Vertex *b) {
	const Common::Point *v1, *v2, *w1, *w2;

	// Check for comparison of the same edge
	if (a == b)
		return false;

	// We can assume that the sweeping line intersects both edges and
	// that the edges do not properly intersect

	// Order vertices clockwise so we know vertex_cur is to the right of
	// directed edges (v1, v2) and (w1, w2)
	clockwise(vertex_cur, a, v1, v2);
	clockwise(vertex_cur, b, w1, w2);

	// At this point we know that one edge must lie entirely to one side
	// of the other, as the edges are not collinear and cannot intersect
	// other than possibly sharing a vertex.

	return ((left_on(*v1, *v2, *w1) && left_on(*v1, *v2, *w2)) || (left_on(*w2, *w1, *v1) && left_on(*w2, *w1, *v2)));
}

/**
 * Determines whether or not a vertex is visible from vertex_cur.
 * @param vertex_cur	the base vertex
 * @param vertex		the vertex
 * @param vertex_prev	the previous vertex in the sort order, or NULL
 * @param visible		true if vertex_prev is visible, false otherwise
 * @param intersected	the list of edges intersected by the sweeping line
 * @return true if vertex is visible from vertex_cur, false otherwise
 */
static bool visible(Vertex *vertex_cur, Vertex *vertex, Vertex *vertex_prev, bool visible, const VertexList &intersected) {
	const Common::Point &p = vertex_cur->v;
	const Common::Point &w = vertex->v;

	// Check if sweeping line intersects the interior of the polygon
	// locally at vertex
	if (inside(p, vertex))
		return false;

	// If vertex_prev is on the sweeping line, then vertex is invisible
	// if vertex_prev is invisible
	if (vertex_prev && !visible && between(p, w, vertex_prev->v))
		return false;

	if (intersected.empty()) {
		// No intersected edges
		return true;
	}

	// Look for the intersected edge that is closest to vertex_cur
	VertexList::const_iterator it = intersected.begin();
	const Vertex *edge = *it++;

	for (; it != intersected.end(); ++it) {
		if (edgeIsCloser(vertex_cur, *it, edge))
			edge = *it;
	}

	const Common::Point *p1, *p2;

	// Check for intersection with sweeping line before vertex
	clockwise(vertex_cur, edge, p1, p2);
	if (left(*p2, *p1, p) && left(*p1, *p2, w))
		return false;

	return true;
}

/**
 * Returns a list of all vertices that are visible from a particular vertex.
 * @param s				the pathfinding state
 * @param vertex_cur	the vertex
 * @return list of vertices that are visible from vert
 */
static VertexList *visible_vertices(PathfindingState *s, Vertex *vertex_cur) {
	// List of edges intersected by the sweeping line
	VertexList intersected;
	VertexList *visVerts = new VertexList();
	const Common::Point &p = vertex_cur->v;

	// Sort vertices by angle (first) and distance (second)
	s_vertex_cur = vertex_cur;
	qsort(s->vertex_index, s->vertices, sizeof(Vertex *), vertex_compare);

	for (PolygonList::iterator it = s->polygons.begin(); it != s->polygons.end(); ++it) {
		Polygon *polygon = *it;
		Vertex *vertex;
		vertex = polygon->vertices.first();

		// Check that there is more than one vertex.
		if (VERTEX_HAS_EDGES(vertex)) {
			CLIST_FOREACH(vertex, &polygon->vertices) {
				const Common::Point *high, *low;

				// Add edges that intersect the initial position of the sweeping line
				clockwise(vertex_cur, vertex, high, low);

				if ((high->y < p.y) && (low->y >= p.y) && (*low != p))
					intersected.push_front(vertex);
			}
		}
	}

	int is_visible = 1;

	// The first vertex will be s_vertex_cur, so we skip it
	for (int i = 1; i < s->vertices; i++) {
		Vertex *v1;

		// Compute visibility of vertex_index[i]
		assert(vertex_cur == s_vertex_cur);	// FIXME: We should be able to replace s_vertex_cur by vertex_cur
		is_visible = visible(s_vertex_cur, s->vertex_index[i], s->vertex_index[i - 1], is_visible, intersected);

		// Update visibility matrix
		if (is_visible)
			visVerts->push_front(s->vertex_index[i]);

		// Delete anti-clockwise edges from list
		v1 = CLIST_PREV(s->vertex_index[i]);
		if (left(p, s->vertex_index[i]->v, v1->v))
			intersected.remove(v1);

		v1 = CLIST_NEXT(s->vertex_index[i]);
		if (left(p, s->vertex_index[i]->v, v1->v))
			intersected.remove(s->vertex_index[i]);

		// Add clockwise edges of collinear vertices when sweeping line moves
		if ((i < s->vertices - 1) && !collinear(p, s->vertex_index[i]->v, s->vertex_index[i + 1]->v)) {
			int j;
			for (j = i; (j >= 1) && collinear(p, s->vertex_index[i]->v, s->vertex_index[j]->v); j--) {
				v1 = CLIST_PREV(s->vertex_index[j]);
				if (left(s->vertex_index[j]->v, p, v1->v))
					intersected.push_front(v1);

				v1 = CLIST_NEXT(s->vertex_index[j]);
				if (left(s->vertex_index[j]->v, p, v1->v))
					intersected.push_front(s->vertex_index[j]);
			}
		}
	}

	s_vertex_cur = 0;

	return visVerts;
}

#else

/**
 * Returns a list of all vertices that are visible from a particular vertex.
 * @param s				the pathfinding state
 * @param vertex_cur	the vertex
 * @return list of vertices that are visible from vert
 */
static VertexList *visible_vertices(PathfindingState *s, Vertex *vertex_cur) {
	VertexList *visVerts = new VertexList();

	for (int i = 0; i < s->vertices; i++) {
		Vertex *vertex = s->vertex_index[i];

		// Make sure we don't intersect a polygon locally at the vertices
		if ((vertex == vertex_cur) || (inside(vertex->v, vertex_cur)) || (inside(vertex_cur->v, vertex)))
			continue;

		// Check for intersecting edges
		int j;
		for (j = 0; j < s->vertices; j++) {
			Vertex *edge = s->vertex_index[j];
			if (VERTEX_HAS_EDGES(edge)) {
				if (between(vertex_cur->v, vertex->v, edge->v)) {
					// If we hit a vertex, make sure we can pass through it without intersecting its polygon
					if ((inside(vertex_cur->v, edge)) || (inside(vertex->v, edge)))
						break;

					// This edge won't properly intersect, so we continue
					continue;
				}

				if (intersect_proper(vertex_cur->v, vertex->v, edge->v, CLIST_NEXT(edge)->v))
					break;
			}
		}

		if (j == s->vertices)
			visVerts->push_front(vertex);
	}

	return visVerts;
}

#endif // OLD_PATHFINDING

bool PathfindingState::pointOnScreenBorder(const Common::Point &p) {
	// Determines if a point lies on the screen border
	// Parameters: (const Common::Point &) p: The point
	// Returns   : (int) true if p lies on the screen border, false otherwise
	return (p.x == 0) || (p.x == _width - 1) || (p.y == 0) || (p.y == _height - 1);
}

bool PathfindingState::edgeOnScreenBorder(const Common::Point &p, const Common::Point &q) {
	// Determines if an edge lies on the screen border
	// Parameters: (const Common::Point &) p, q: The edge (p, q)
	// Returns   : (int) true if (p, q) lies on the screen border, false otherwise
	return ((p.x == 0 && q.x == 0) || (p.y == 0 && q.y == 0)
			|| ((p.x == _width - 1) && (q.x == _width - 1))
			|| ((p.y == _height - 1) && (q.y == _height - 1)));
}

static int find_free_point(FloatPoint f, Polygon *polygon, Common::Point *ret) {
	// Searches for a nearby point that is not contained in a polygon
	// Parameters: (FloatPoint) f: The pointf to search nearby
	//             (Polygon *) polygon: The polygon
	// Returns   : (int) PF_OK on success, PF_FATAL otherwise
	//             (Common::Point) *ret: The non-contained point on success
	Common::Point p;

	// Try nearest point first
	p = Common::Point((int)floor(f.x + 0.5), (int)floor(f.y + 0.5));

	if (contained(p, polygon) != CONT_INSIDE) {
		*ret = p;
		return PF_OK;
	}

	p = Common::Point((int)floor(f.x), (int)floor(f.y));

	// Try (x, y), (x + 1, y), (x , y + 1) and (x + 1, y + 1)
	if (contained(p, polygon) == CONT_INSIDE) {
		p.x++;
		if (contained(p, polygon) == CONT_INSIDE) {
			p.y++;
			if (contained(p, polygon) == CONT_INSIDE) {
				p.x--;
				if (contained(p, polygon) == CONT_INSIDE)
					return PF_FATAL;
			}
		}
	}

	*ret = p;
	return PF_OK;
}

int PathfindingState::findNearPoint(const Common::Point &p, Polygon *polygon, Common::Point *ret) {
	// Computes the near point of a point contained in a polygon
	// Parameters: (const Common::Point &) p: The point
	//             (Polygon *) polygon: The polygon
	// Returns   : (int) PF_OK on success, PF_FATAL otherwise
	//             (Common::Point) *ret: The near point of p in polygon on success
	Vertex *vertex;
	FloatPoint near_p;
	uint32 dist = HUGE_DISTANCE;

	CLIST_FOREACH(vertex, &polygon->vertices) {
		const Common::Point &p1 = vertex->v;
		const Common::Point &p2 = CLIST_NEXT(vertex)->v;
		float u;
		FloatPoint new_point;
		uint32 new_dist;

		// Ignore edges on the screen border, except for contained access polygons
		if ((polygon->type != POLY_CONTAINED_ACCESS) && (edgeOnScreenBorder(p1, p2)))
			continue;

		// Compute near point
		u = ((p.x - p1.x) * (p2.x - p1.x) + (p.y - p1.y) * (p2.y - p1.y)) / (float)p1.sqrDist(p2);

		// Clip to edge
		if (u < 0.0f)
			u = 0.0f;
		if (u > 1.0f)
			u = 1.0f;

		new_point.x = p1.x + u * (p2.x - p1.x);
		new_point.y = p1.y + u * (p2.y - p1.y);

		new_dist = p.sqrDist(new_point.toPoint());

		if (new_dist < dist) {
			near_p = new_point;
			dist = new_dist;
		}
	}

	// Find point not contained in polygon
	return find_free_point(near_p, polygon, ret);
}

static int intersection(const Common::Point &a, const Common::Point &b, Vertex *vertex, FloatPoint *ret) {
	// Computes the intersection point of a line segment and an edge (not
	// including the vertices themselves)
	// Parameters: (const Common::Point &) a, b: The line segment (a, b)
	//             (Vertex *) vertex: The first vertex of the edge
	// Returns   : (int) FP_OK on success, PF_ERROR otherwise
	//             (FloatPoint) *ret: The intersection point
	// Parameters of parametric equations
	float s, t;
	// Numerator and denominator of equations
	float num, denom;
	const Common::Point &c = vertex->v;
	const Common::Point &d = CLIST_NEXT(vertex)->v;

	denom = a.x * (float)(d.y - c.y) + b.x * (float)(c.y - d.y) +
	        d.x * (float)(b.y - a.y) + c.x * (float)(a.y - b.y);

	if (denom == 0.0)
		// Segments are parallel, no intersection
		return PF_ERROR;

	num = a.x * (float)(d.y - c.y) + c.x * (float)(a.y - d.y) + d.x * (float)(c.y - a.y);

	s = num / denom;

	num = -(a.x * (float)(c.y - b.y) + b.x * (float)(a.y - c.y) + c.x * (float)(b.y - a.y));

	t = num / denom;

	if ((0.0 <= s) && (s <= 1.0) && (0.0 < t) && (t < 1.0)) {
		// Intersection found
		ret->x = a.x + s * (b.x - a.x);
		ret->y = a.y + s * (b.y - a.y);
		return PF_OK;
	}

	return PF_ERROR;
}

static int nearest_intersection(PathfindingState *s, const Common::Point &p, const Common::Point &q, Common::Point *ret) {
	// Computes the nearest intersection point of a line segment and the polygon
	// set. Intersection points that are reached from the inside of a polygon
	// are ignored as are improper intersections which do not obstruct
	// visibility
	// Parameters: (PathfindingState *) s: The pathfinding state
	//             (const Common::Point &) p, q: The line segment (p, q)
	// Returns   : (int) PF_OK on success, PF_ERROR when no intersections were
	//                   found, PF_FATAL otherwise
	//             (Common::Point) *ret: On success, the closest intersection point
	Polygon *polygon = 0;
	FloatPoint isec;
	Polygon *ipolygon = 0;
	uint32 dist = HUGE_DISTANCE;

	for (PolygonList::iterator it = s->polygons.begin(); it != s->polygons.end(); ++it) {
		polygon = *it;
		Vertex *vertex;

		CLIST_FOREACH(vertex, &polygon->vertices) {
			uint32 new_dist;
			FloatPoint new_isec;

			// Check for intersection with vertex
			if (between(p, q, vertex->v)) {
				// Skip this vertex if we hit it from the
				// inside of the polygon
				if (inside(q, vertex)) {
					new_isec.x = vertex->v.x;
					new_isec.y = vertex->v.y;
				} else
					continue;
			} else {
				// Check for intersection with edges

				// Skip this edge if we hit it from the
				// inside of the polygon
				if (!left(vertex->v, CLIST_NEXT(vertex)->v, q))
					continue;

				if (intersection(p, q, vertex, &new_isec) != PF_OK)
					continue;
			}

			new_dist = p.sqrDist(new_isec.toPoint());
			if (new_dist < dist) {
				ipolygon = polygon;
				isec = new_isec;
				dist = new_dist;
			}
		}
	}

	if (dist == HUGE_DISTANCE)
		return PF_ERROR;

	// Find point not contained in polygon
	return find_free_point(isec, ipolygon, ret);
}

/**
 * Checks whether a point is nearby a contained-access polygon (distance 1 pixel)
 * @param point			the point
 * @param polygon		the contained-access polygon
 * @return true when point is nearby polygon, false otherwise
 */
static bool nearbyPolygon(const Common::Point &point, Polygon *polygon) {
	assert(polygon->type == POLY_CONTAINED_ACCESS);

	return ((contained(Common::Point(point.x, point.y + 1), polygon) != CONT_INSIDE)
			|| (contained(Common::Point(point.x, point.y - 1), polygon) != CONT_INSIDE)
			|| (contained(Common::Point(point.x + 1, point.y), polygon) != CONT_INSIDE)
			|| (contained(Common::Point(point.x - 1, point.y), polygon) != CONT_INSIDE));
}

/**
 * Checks that the start point is in a valid position, and takes appropriate action if it's not.
 * @param s				the pathfinding state
 * @param start			the start point
 * @return a valid start point on success, NULL otherwise
 */
static Common::Point *fixup_start_point(PathfindingState *s, const Common::Point &start) {
	PolygonList::iterator it = s->polygons.begin();
	Common::Point *new_start = new Common::Point(start);

	while (it != s->polygons.end()) {
		int cont = contained(start, *it);
		int type = (*it)->type;

		switch (type) {
		case POLY_TOTAL_ACCESS:
			// Remove totally accessible polygons that contain the start point
			if (cont != CONT_OUTSIDE) {
				delete *it;
				it = s->polygons.erase(it);
				continue;
			}
			break;
		case POLY_CONTAINED_ACCESS:
			// Remove contained access polygons that do not contain
			// the start point (containment test is inverted here).
			// SSCI appears to be using a small margin of error here,
			// so we do the same.
			if ((cont == CONT_INSIDE) && !nearbyPolygon(start, *it)) {
				delete *it;
				it = s->polygons.erase(it);
				continue;
			}
			// Fall through
		case POLY_BARRED_ACCESS:
		case POLY_NEAREST_ACCESS:
			if (cont == CONT_INSIDE) {
				if (s->_prependPoint != NULL) {
					// We shouldn't get here twice
					warning("AvoidPath: start point is contained in multiple polygons");
					continue;
				}

				if (s->findNearPoint(start, (*it), new_start) != PF_OK) {
					delete new_start;
					return NULL;
				}

				if ((type == POLY_BARRED_ACCESS) || (type == POLY_CONTAINED_ACCESS))
					warning("AvoidPath: start position at unreachable location");

				// The original start position is in an invalid location, so we
				// use the moved point and add the original one to the final path
				// later on.
				s->_prependPoint = new Common::Point(start);
			}
		}

		++it;
	}

	return new_start;
}

/**
 * Checks that the end point is in a valid position, and takes appropriate action if it's not.
 * @param s				the pathfinding state
 * @param end			the end point
 * @return a valid end point on success, NULL otherwise
 */
static Common::Point *fixup_end_point(PathfindingState *s, const Common::Point &end) {
	PolygonList::iterator it = s->polygons.begin();
	Common::Point *new_end = new Common::Point(end);

	while (it != s->polygons.end()) {
		int cont = contained(end, *it);
		int type = (*it)->type;

		switch (type) {
		case POLY_TOTAL_ACCESS:
			// Remove totally accessible polygons that contain the end point
			if (cont != CONT_OUTSIDE) {
				delete *it;
				it = s->polygons.erase(it);
				continue;
			}
			break;
		case POLY_CONTAINED_ACCESS:
		case POLY_BARRED_ACCESS:
		case POLY_NEAREST_ACCESS:
			if (cont != CONT_OUTSIDE) {
				if (s->_appendPoint != NULL) {
					// We shouldn't get here twice
					warning("AvoidPath: end point is contained in multiple polygons");
					continue;
				}

				// The original end position is in an invalid location, so we move the point
				if (s->findNearPoint(end, (*it), new_end) != PF_OK) {
					delete new_end;
					return NULL;
				}

				// For near-point access polygons we need to add the original end point
				// to the path after pathfinding.
				if (type == POLY_NEAREST_ACCESS)
					s->_appendPoint = new Common::Point(end);
			}
		}

		++it;
	}

	return new_end;
}

static Vertex *merge_point(PathfindingState *s, const Common::Point &v) {
	// Merges a point into the polygon set. A new vertex is allocated for this
	// point, unless a matching vertex already exists. If the point is on an
	// already existing edge that edge is split up into two edges connected by
	// the new vertex
	// Parameters: (PathfindingState *) s: The pathfinding state
	//             (const Common::Point &) v: The point to merge
	// Returns   : (Vertex *) The vertex corresponding to v
	Vertex *vertex;
	Vertex *v_new;
	Polygon *polygon;

	// Check for already existing vertex
	for (PolygonList::iterator it = s->polygons.begin(); it != s->polygons.end(); ++it) {
		polygon = *it;
		CLIST_FOREACH(vertex, &polygon->vertices) {
			if (vertex->v == v)
				return vertex;
		}
	}

	v_new = new Vertex(v);

	// Check for point being on an edge
	for (PolygonList::iterator it = s->polygons.begin(); it != s->polygons.end(); ++it) {
		polygon = *it;
		// Skip single-vertex polygons
		if (VERTEX_HAS_EDGES(polygon->vertices.first())) {
			CLIST_FOREACH(vertex, &polygon->vertices) {
				Vertex *next = CLIST_NEXT(vertex);

				if (between(vertex->v, next->v, v)) {
					// Split edge by adding vertex
					polygon->vertices.insertAfter(vertex, v_new);
					return v_new;
				}
			}
		}
	}

	// Add point as single-vertex polygon
	polygon = new Polygon(POLY_BARRED_ACCESS);
	polygon->vertices.insertHead(v_new);
	s->polygons.push_front(polygon);

	return v_new;
}

static Polygon *convert_polygon(EngineState *s, reg_t polygon) {
	// Converts an SCI polygon into a Polygon
	// Parameters: (EngineState *) s: The game state
	//             (reg_t) polygon: The SCI polygon to convert
	// Returns   : (Polygon *) The converted polygon, or NULL on error
	SegManager *segMan = s->_segMan;
	int i;
	reg_t points = GET_SEL32(segMan, polygon, points);
	int size = GET_SEL32(segMan, polygon, size).toUint16();

#ifdef ENABLE_SCI32
	// SCI32 stores the actual points in the data property of points (in a new array)
	if (segMan->isHeapObject(points))
		points = GET_SEL32(segMan, points, data);
#endif

	if (size == 0) {
		// If the polygon has no vertices, we skip it
		return NULL;
	}

	Polygon *poly = new Polygon(GET_SEL32(segMan, polygon, type).toUint16());

	int skip = 0;

	// WORKAROUND: broken polygon in lsl1sci, room 350, after opening elevator
	// Polygon has 17 points but size is set to 19
	if ((size == 19) && (s->_gameId == "lsl1sci")) {
		if ((s->currentRoomNumber() == 350)
		&& (read_point(segMan, points, 18) == Common::Point(108, 137))) {
			debug(1, "Applying fix for broken polygon in lsl1sci, room 350");
			size = 17;
		}
	}

#ifdef OLD_PATHFINDING
	// WORKAROUND: self-intersecting polygons in ECO, rooms 221, 280 and 300
	if ((size == 11) && (s->_gameId == "ecoquest")) {
		if ((s->currentRoomNumber() == 300)
		&& (read_point(segMan, points, 10) == Common::Point(221, 0))) {
			debug(1, "Applying fix for self-intersecting polygon in ECO, room 300");
			size = 10;
		}
	}
	if ((size == 12) && (s->_gameId == "ecoquest")) {
		if ((s->currentRoomNumber() == 280)
		&& (read_point(segMan, points, 11) == Common::Point(238, 189))) {
			debug(1, "Applying fix for self-intersecting polygon in ECO, room 280");
			size = 10;
		}
	}
	if ((size == 16) && (s->_gameId == "ecoquest")) {
		if ((s->currentRoomNumber() == 221)
		&& (read_point(segMan, points, 1) == Common::Point(419, 175))) {
			debug(1, "Applying fix for self-intersecting polygon in ECO, room 221");
			// Swap the first two points
			poly->vertices.insertHead(new Vertex(read_point(segMan, points, 1)));
			poly->vertices.insertHead(new Vertex(read_point(segMan, points, 0)));
			skip = 2;
		}
	}
#endif

	for (i = skip; i < size; i++) {
#ifdef OLD_PATHFINDING
		if (size == 35 && (i == 20 || i == 21) && s->_gameId == "sq1sci" &&
			s->currentRoomNumber() == 66) {
			if (i == 20 && read_point(segMan, points, 20) == Common::Point(0, 104)) {
				debug(1, "Applying fix for self-intersecting polygon in SQ1, room 66");
				Vertex *vertex = new Vertex(Common::Point(1, 104));
				poly->vertices.insertHead(vertex);
				continue;
			} else if (i == 21 && read_point(segMan, points, 21) == Common::Point(0, 110)) {
				debug(1, "Applying fix for self-intersecting polygon in SQ1, room 66");
				Vertex *vertex = new Vertex(Common::Point(1, 110));
				poly->vertices.insertHead(vertex);
				continue;
			}
		}
#endif
		Vertex *vertex = new Vertex(read_point(segMan, points, i));
		poly->vertices.insertHead(vertex);
	}

	fix_vertex_order(poly);

	return poly;
}

#ifdef OLD_PATHFINDING
// WORKAROUND: intersecting polygons in Longbow, room 210.
static void fixLongbowRoom210(PathfindingState *s, const Common::Point &start, const Common::Point &end) {
	Polygon *barred = NULL;
	Polygon *total = NULL;

	// Find the intersecting polygons
	for (PolygonList::iterator it = s->polygons.begin(); it != s->polygons.end(); ++it) {
		Polygon *polygon = *it;
		assert(polygon);

		if ((polygon->type == POLY_BARRED_ACCESS) && (polygon->vertices.size() == 11)
		&& (polygon->vertices.first()->v == Common::Point(319, 161)))
			barred = polygon;
		else if ((polygon->type == POLY_TOTAL_ACCESS) && (polygon->vertices.size() == 8)
		&& (polygon->vertices.first()->v == Common::Point(313, 58)))
			total = polygon;
	}

	if (!barred || !total)
		return;

	debug(1, "[avoidpath] Applying fix for intersecting polygons in Longbow, room 210");

	// If the start or end point is contained in the total access polygon, removing that
	// polygon is sufficient. Otherwise we merge the total and barred access polygons.
	bool both_outside = (contained(start, total) == CONT_OUTSIDE) && (contained(end, total) == CONT_OUTSIDE);

	s->polygons.remove(total);
	delete total;

	if (both_outside) {
		int points[28] = {
			224, 159, 223, 162 ,194, 173 ,107, 173, 74, 162, 67, 156, 2, 58,
			63, 160, 0, 160, 0, 0, 319, 0, 319, 161, 228, 161, 313, 58
		};

		s->polygons.remove(barred);
		delete barred;

		barred = new Polygon(POLY_BARRED_ACCESS);

		for (int i = 0; i < 14; i++) {
			Vertex *vertex = new Vertex(Common::Point(points[i * 2], points[i * 2 + 1]));
			barred->vertices.insertHead(vertex);
		}

		s->polygons.push_front(barred);
	}
}
#endif

static void change_polygons_opt_0(PathfindingState *s) {
	// Changes the polygon list for optimization level 0 (used for keyboard
	// support). Totally accessible polygons are removed and near-point
	// accessible polygons are changed into totally accessible polygons.
	// Parameters: (PathfindingState *) s: The pathfinding state

	PolygonList::iterator it = s->polygons.begin();
	while (it != s->polygons.end()) {
		Polygon *polygon = *it;
		assert(polygon);

		if (polygon->type == POLY_TOTAL_ACCESS) {
			delete polygon;
			it = s->polygons.erase(it);
		} else {
			if (polygon->type == POLY_NEAREST_ACCESS)
				polygon->type = POLY_TOTAL_ACCESS;
			++it;
		}
	}
}

static PathfindingState *convert_polygon_set(EngineState *s, reg_t poly_list, Common::Point start, Common::Point end, int width, int height, int opt) {
	// Converts the SCI input data for pathfinding
	// Parameters: (EngineState *) s: The game state
	//             (reg_t) poly_list: Polygon list
	//             (Common::Point) start: The start point
	//             (Common::Point) end: The end point
	//             (int) opt: Optimization level (0, 1 or 2)
	// Returns   : (PathfindingState *) On success a newly allocated pathfinding state,
	//                            NULL otherwise
	SegManager *segMan = s->_segMan;
	Polygon *polygon;
	int err;
	int count = 0;
	PathfindingState *pf_s = new PathfindingState(width, height);

	// Convert all polygons
	if (poly_list.segment) {
		List *list = s->_segMan->lookupList(poly_list);
		Node *node = s->_segMan->lookupNode(list->first);

		while (node) {
			Node *dup = s->_segMan->lookupNode(list->first);

			// Workaround for game bugs that put a polygon in the list more than once
			while (dup != node) {
#ifdef OLD_PATHFINDING
				if (polygons_equal(s->_segMan, node->value, dup->value)) {
					warning("[avoidpath] Ignoring duplicate polygon");
					break;
				}
#endif
				dup = s->_segMan->lookupNode(dup->succ);
			}

			if (dup == node) {
				// Polygon is not a duplicate, so convert it
				polygon = convert_polygon(s, node->value);

				if (polygon) {
					pf_s->polygons.push_back(polygon);
					count += GET_SEL32(segMan, node->value, size).toUint16();
				}
			}

			node = s->_segMan->lookupNode(node->succ);
		}
	}

	if (opt == 0) {
		Common::Point intersection;

		// Keyboard support
		// FIXME: We don't need to dijkstra for keyboard support as we currently do
		change_polygons_opt_0(pf_s);

		// Find nearest intersection
		err = nearest_intersection(pf_s, start, end, &intersection);

		if (err == PF_FATAL) {
			warning("AvoidPath: fatal error finding nearest intersection");
			delete pf_s;
			return NULL;
		}

		if (err == PF_OK) {
			// Intersection was found, prepend original start position after pathfinding
			pf_s->_prependPoint = new Common::Point(start);
			// Merge new start point into polygon set
			pf_s->vertex_start = merge_point(pf_s, intersection);
		} else {
			// Otherwise we proceed with the original start point
			pf_s->vertex_start = merge_point(pf_s, start);
		}
		// Merge end point into polygon set
		pf_s->vertex_end = merge_point(pf_s, end);
	} else {
		Common::Point *new_start = fixup_start_point(pf_s, start);

		if (!new_start) {
			warning("AvoidPath: Couldn't fixup start position for pathfinding");
			delete pf_s;
			return NULL;
		}

		Common::Point *new_end = fixup_end_point(pf_s, end);

		if (!new_end) {
			warning("AvoidPath: Couldn't fixup end position for pathfinding");
			delete pf_s;
			return NULL;
		}

		// WORKAROUND LSL5 room 660. Priority glitch due to us choosing a different path
		// than SSCI. Happens when Patti walks to the control room.
		if ((s->_gameId == "lsl5") && (s->currentRoomNumber() == 660) && (Common::Point(67, 131) == *new_start) && (Common::Point(229, 101) == *new_end)) {
			debug(1, "[avoidpath] Applying fix for priority problem in LSL5, room 660");
			pf_s->_prependPoint = new_start;
			new_start = new Common::Point(77, 107);
		}

#ifdef OLD_PATHFINDING
		if (s->_gameId == "longbow" && s->currentRoomNumber() == 210)
				fixLongbowRoom210(pf_s, *new_start, *new_end);
#endif

		// Merge start and end points into polygon set
		pf_s->vertex_start = merge_point(pf_s, *new_start);
		pf_s->vertex_end = merge_point(pf_s, *new_end);

		delete new_start;
		delete new_end;
	}

	// Allocate and build vertex index
	pf_s->vertex_index = (Vertex**)malloc(sizeof(Vertex *) * (count + 2));

	count = 0;

	for (PolygonList::iterator it = pf_s->polygons.begin(); it != pf_s->polygons.end(); ++it) {
		polygon = *it;
		Vertex *vertex;

		CLIST_FOREACH(vertex, &polygon->vertices) {
			pf_s->vertex_index[count++] = vertex;
		}
	}

	pf_s->vertices = count;

	return pf_s;
}

#ifdef OLD_PATHFINDING
static bool intersect(const Common::Point &a, const Common::Point &b, const Common::Point &c, const Common::Point &d) {
	// Determines whether or not two line segments intersect
	// Parameters: (const Common::Point &) a, b: The line segment (a, b)
	//             (const Common::Point &) c, d: The line segment (c, d)
	// Returns   : (int) true if (a, b) intersects (c, d), false otherwise
	if (intersect_proper(a, b, c, d))
		return true;

	return between(a, b, c) || between(a, b, d) || between(c, d, a) || between(c, d, b);
}

static int intersecting_polygons(PathfindingState *s) {
	// Detects (self-)intersecting polygons
	// Parameters: (PathfindingState *) s: The pathfinding state
	// Returns   : (int) 1 if s contains (self-)intersecting polygons, 0 otherwise
	int i, j;

	for (i = 0; i < s->vertices; i++) {
		Vertex *v1 = s->vertex_index[i];
		if (!VERTEX_HAS_EDGES(v1))
			continue;
		for (j = i + 1; j < s->vertices; j++) {
			Vertex *v2 = s->vertex_index[j];
			if (!VERTEX_HAS_EDGES(v2))
				continue;

			// Skip neighbouring edges
			if ((CLIST_NEXT(v1) == v2) || CLIST_PREV(v1) == v2)
				continue;

			if (intersect(v1->v, CLIST_NEXT(v1)->v,
			              v2->v, CLIST_NEXT(v2)->v))
				return 1;
		}
	}

	return 0;
}
#endif

/**
 * Computes a shortest path from vertex_start to vertex_end. The caller can
 * construct the resulting path by following the path_prev links from
 * vertex_end back to vertex_start. If no path exists vertex_end->path_prev
 * will be NULL
 * Parameters: (PathfindingState *) s: The pathfinding state
 *             (bool) avoidScreenEdge: Avoid screen edges (default behavior)
 */
static void AStar(PathfindingState *s, bool avoidScreenEdge) {
	// Vertices of which the shortest path is known
	VertexList closedSet;

	// The remaining vertices
	VertexList openSet;

	openSet.push_front(s->vertex_start);
	s->vertex_start->costG = 0;
	s->vertex_start->costF = (uint32)sqrt((float)s->vertex_start->v.sqrDist(s->vertex_end->v));

	while (!openSet.empty()) {
		// Find vertex in open set with lowest F cost
		VertexList::iterator vertex_min_it = openSet.end();
		Vertex *vertex_min = 0;
		uint32 min = HUGE_DISTANCE;

		for (VertexList::iterator it = openSet.begin(); it != openSet.end(); ++it) {
			Vertex *vertex = *it;
			if (vertex->costF < min) {
				vertex_min_it = it;
				vertex_min = *vertex_min_it;
				min = vertex->costF;
			}
		}

		// Check if we are done
		if (vertex_min == s->vertex_end)
			break;

		// Move vertex from set open to set closed
		closedSet.push_front(vertex_min);
		openSet.erase(vertex_min_it);

		VertexList *visVerts = visible_vertices(s, vertex_min);

		for (VertexList::iterator it = visVerts->begin(); it != visVerts->end(); ++it) {
			uint32 new_dist;
			Vertex *vertex = *it;

			if (closedSet.contains(vertex))
				continue;

			if (avoidScreenEdge) {
				// Avoid plotting path along screen edge
				if ((vertex != s->vertex_end) && s->pointOnScreenBorder(vertex->v))
					continue;
			}

			if (!openSet.contains(vertex))
				openSet.push_front(vertex);

			new_dist = vertex_min->costG + (uint32)sqrt((float)vertex_min->v.sqrDist(vertex->v));
			if (new_dist < vertex->costG) {
				vertex->costG = new_dist;
				vertex->costF = vertex->costG + (uint32)sqrt((float)vertex->v.sqrDist(s->vertex_end->v));
				vertex->path_prev = vertex_min;
			}
		}

		delete visVerts;
	}

	if (openSet.empty())
		warning("[avoidpath] End point (%i, %i) is unreachable", s->vertex_end->v.x, s->vertex_end->v.y);
}

static reg_t allocateOutputArray(SegManager *segMan, int size) {
	reg_t addr;

#ifdef ENABLE_SCI32
	if (getSciVersion() >= SCI_VERSION_2) {
		SciArray<reg_t> *array = segMan->allocateArray(&addr);
		assert(array);
		array->setType(0);
		array->setSize(size * 2);
		return addr;
	}
#endif

	segMan->allocDynmem(POLY_POINT_SIZE * size, AVOIDPATH_DYNMEM_STRING, &addr);
	return addr;
}

static reg_t output_path(PathfindingState *p, EngineState *s) {
	// Stores the final path in newly allocated dynmem
	// Parameters: (PathfindingState *) p: The pathfinding state
	//             (EngineState *) s: The game state
	// Returns   : (reg_t) Pointer to dynmem containing path
	int path_len = 0;
	reg_t output;
	Vertex *vertex = p->vertex_end;
	int unreachable = vertex->path_prev == NULL;

	if (!unreachable) {
		while (vertex) {
			// Compute path length
			path_len++;
			vertex = vertex->path_prev;
		}
	}

	// Allocate memory for path, plus 3 extra for appended point, prepended point and sentinel
	output = allocateOutputArray(s->_segMan, path_len + 3);
	SegmentRef arrayRef = s->_segMan->dereference(output);
	assert(arrayRef.isValid() && !arrayRef.skipByte);

	if (unreachable) {
		// If pathfinding failed we only return the path up to vertex_start

		if (p->_prependPoint)
			writePoint(arrayRef, 0, *p->_prependPoint);
		else
			writePoint(arrayRef, 0, p->vertex_start->v);

		writePoint(arrayRef, 1, p->vertex_start->v);
		writePoint(arrayRef, 2, Common::Point(POLY_LAST_POINT, POLY_LAST_POINT));

		return output;
	}

	int offset = 0;

	if (p->_prependPoint)
		writePoint(arrayRef, offset++, *p->_prependPoint);

	vertex = p->vertex_end;
	for (int i = path_len - 1; i >= 0; i--) {
		writePoint(arrayRef, offset + i, vertex->v);
		vertex = vertex->path_prev;
	}
	offset += path_len;

	if (p->_appendPoint)
		writePoint(arrayRef, offset++, *p->_appendPoint);

	// Sentinel
	writePoint(arrayRef, offset, Common::Point(POLY_LAST_POINT, POLY_LAST_POINT));

#ifdef DEBUG_AVOIDPATH
	printf("[avoidpath] Returning path:");
	for (int i = 0; i < offset; i++) {
		Common::Point pt = read_point(s->_segMan, output, i);
		printf(" (%i, %i)", pt.x, pt.y);
	}
	printf("\n");
#endif

	return output;
}

reg_t kAvoidPath(EngineState *s, int argc, reg_t *argv) {
	Common::Point start = Common::Point(argv[0].toSint16(), argv[1].toSint16());

	switch (argc) {

	case 3 : {
		reg_t retval;
		Polygon *polygon = convert_polygon(s, argv[2]);

		if (!polygon)
			return NULL_REG;

		// Override polygon type to prevent inverted result for contained access polygons
		polygon->type = POLY_BARRED_ACCESS;

		retval = make_reg(0, contained(start, polygon) != CONT_OUTSIDE);
		delete polygon;
		return retval;
	}
	case 6 :
	case 7 :
	case 8 : {
		Common::Point end = Common::Point(argv[2].toSint16(), argv[3].toSint16());
		reg_t poly_list, output;
		int width, height, opt = 1;

		if (getSciVersion() >= SCI_VERSION_2) {
			if (argc < 7)
				error("[avoidpath] Not enough arguments");

			poly_list = GET_SEL32(s->_segMan, argv[4], elements);
			width = argv[5].toUint16();
			height = argv[6].toUint16();
			if (argc > 7)
				opt = argv[7].toUint16();
		} else {
			// SCI1.1 and older games always ran with an internal resolution of 320x200
			poly_list = argv[4];
			width = 320;
			height = 190;
			if (argc > 6)
				opt = argv[6].toUint16();
		}

#ifdef DEBUG_AVOIDPATH
		printf("[avoidpath] Pathfinding input:\n");
		draw_point(s, start, 1, width, height);
		draw_point(s, end, 0, width, height);

		if (poly_list.segment) {
			print_input(s, poly_list, start, end, opt);
			draw_input(s, poly_list, start, end, opt, width, height);
		}

		// Update the whole screen
		s->_gui->graphUpdateBox(Common::Rect(0, 0, width - 1, height - 1));
#endif

		PathfindingState *p = convert_polygon_set(s, poly_list, start, end, width, height, opt);

#ifdef OLD_PATHFINDING
		if (p && intersecting_polygons(p)) {
			warning("[avoidpath] input set contains (self-)intersecting polygons");
			delete p;
			p = NULL;
		}
#endif

		if (!p) {
			printf("[avoidpath] Error: pathfinding failed for following input:\n");
			print_input(s, poly_list, start, end, opt);
			printf("[avoidpath] Returning direct path from start point to end point\n");
			output = allocateOutputArray(s->_segMan, 3);
			SegmentRef arrayRef = s->_segMan->dereference(output);
			assert(arrayRef.isValid() && !arrayRef.skipByte);

			writePoint(arrayRef, 0, start);
			writePoint(arrayRef, 1, end);
			writePoint(arrayRef, 2, Common::Point(POLY_LAST_POINT, POLY_LAST_POINT));

			return output;
		}

		// Apply Dijkstra, avoiding screen edges
		AStar(p, true);

		output = output_path(p, s);
		delete p;

		// Memory is freed by explicit calls to Memory
		return output;
	}

	default:
		warning("Unknown AvoidPath subfunction %d", argc);
		return NULL_REG;
		break;
	}
}

} // End of namespace Sci
