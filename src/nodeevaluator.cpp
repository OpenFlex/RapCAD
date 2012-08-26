/*
 *   RapCAD - Rapid prototyping CAD IDE (www.rapcad.org)
 *   Copyright (C) 2010-2012 Giles Bathgate
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QVector>
#include "nodeevaluator.h"
#include "cgalimport.h"

NodeEvaluator::NodeEvaluator(QTextStream& s) : output(s)
{
}

NodeEvaluator::~NodeEvaluator()
{
	Node::cleanup();
}

void NodeEvaluator::visit(PrimitiveNode* n)
{
#if USE_CGAL
	CGALPrimitive* cp = new CGALPrimitive();
	foreach(Polygon p, n->getPolygons()) {
		cp->createPolygon();
		foreach(Point pt, p) {
			double x,y,z;
			pt.getXYZ(x,y,z);
			cp->appendVertex(CGAL::Point3(x,y,z));
		}
	}
	result=cp->buildVolume();
#endif
}

void NodeEvaluator::visit(PolylineNode* n)
{
#if USE_CGAL
	QVector<CGAL::Point3> pl;
	foreach(Point p,n->getPoints()) {
		double x,y,z;
		p.getXYZ(x,y,z);
		pl.append(CGAL::Point3(x,y,z));
	}
	result=new CGALPrimitive(pl);
#endif
}

void NodeEvaluator::visit(UnionNode* op)
{
	evaluate(op,Union);
}

void NodeEvaluator::visit(DifferenceNode* op)
{
	evaluate(op,Difference);
}

void NodeEvaluator::visit(IntersectionNode* op)
{
	evaluate(op,Intersection);
}

void NodeEvaluator::visit(SymmetricDifferenceNode* op)
{
	evaluate(op,SymmetricDifference);
}

void NodeEvaluator::visit(MinkowskiNode* op)
{
	evaluate(op,Minkowski);
}

void NodeEvaluator::visit(GlideNode* op)
{
#if USE_CGAL
	CGALPrimitive* first=NULL;
	foreach(Node* n, op->getChildren()) {
		n->accept(*this);
		if(!first) {
			CGALExplorer explorer(result);
			QList<CGAL::Point3> points = explorer.getPoints();

			QVector<CGAL::Point3> pl;
			CGAL::Point3 fp;
			for(int i=0; i<points.size(); i++) {
				if(i==0)
					fp=points.at(i);
				pl.append(points.at(i));
			}
			if(op->getClosed())
				pl.append(fp);

			first=new CGALPrimitive(pl);
		} else {
			first=first->minkowski(result);
		}
	}

	result=first;
#endif
}

void NodeEvaluator::visit(HullNode* n)
{
#if USE_CGAL
	QList<CGAL::Point3> points;
	foreach(Node* c,n->getChildren()) {
		c->accept(*this);
		CGALExplorer explorer(result);
		foreach(CGAL::Point3 pt,explorer.getPoints())
			points.append(pt);
	}

	CGAL::Polyhedron3 hull;
	CGAL::convex_hull_3(points.begin(),points.end(),hull);

	result=new CGALPrimitive(hull);
#endif
}

void NodeEvaluator::visit(LinearExtrudeNode* op)
{
	evaluate(op,Union);
#if USE_CGAL
	if(result->isFullyDimentional()) {
		QVector<CGAL::Point3> pl;
		pl.append(CGAL::Point3(0,0,0));
		pl.append(CGAL::Point3(0,0,op->getHeight()));
		CGALPrimitive* prim=new CGALPrimitive(pl);
		result=result->minkowski(prim);

	} else {
		CGAL::Kernel3::FT z=op->getHeight();
		CGALExplorer explorer(result);
		CGALPrimitive* prim=explorer.getPrimitive();
		QList<CGALPolygon*> polys=prim->getPolygons();
		CGALPrimitive* n = new CGALPrimitive();

		foreach(CGALPolygon* pg,polys) {
			n->createPolygon();
			bool up =(pg->getNormal().z()>0);
			foreach(CGAL::Point3 pt,pg->getPoints()) {
				if(up)
					n->appendVertex(pt);
				else
					n->prependVertex(pt);
			}
		}

		foreach(CGALExplorer::HalfEdgeHandle h, explorer.getPerimeter()) {
			n->createPolygon();
			n->appendVertex(offset(h->source()->point(),z));
			n->appendVertex(offset(h->target()->point(),z));
			n->appendVertex(h->target()->point());
			n->appendVertex(h->source()->point());
		}

		foreach(CGALPolygon* pg,polys) {
			n->createPolygon();
			bool up =(pg->getNormal().z()>0);
			foreach(CGAL::Point3 pt,pg->getPoints()) {
				if(up)
					n->prependVertex(offset(pt,z));
				else
					n->appendVertex(offset(pt,z));
			}
		}

		result=n->buildVolume();
	}
#endif
}

void NodeEvaluator::visit(RotateExtrudeNode*)
{
}

#if USE_CGAL
CGAL::Point3 NodeEvaluator::offset(const CGAL::Point3& p,CGAL::Kernel3::FT z)
{
	z+=p.z();
	return CGAL::Point3(p.x(),p.y(),z);
}
#endif

void NodeEvaluator::evaluate(Node* op,Operation_e type)
{
#if USE_CGAL
	CGALPrimitive* first=NULL;
	foreach(Node* n, op->getChildren()) {
		n->accept(*this);
		if(!first) {
			first=result;
		} else {
			switch(type) {
			case Union:
				first=first->join(result);
				break;
			case Difference:
				first=first->difference(result);
				break;
			case Intersection:
				first=first->intersection(result);
				break;
			case SymmetricDifference:
				first=first->symmetric_difference(result);
				break;
			case Minkowski:
				first=first->minkowski(result);
				break;
			}
		}
	}

	result=first;
#endif
}

void NodeEvaluator::visit(BoundsNode* n)
{
	evaluate(n,Union);
#if USE_CGAL
	CGALExplorer explorer(result);
	CGAL::Bbox_3 b=explorer.getBounds();

	//TODO move this warning into gcode generation routines when they exist.
	if(b.zmin()!=0.0) {
		QString where = b.zmin()<0.0?" below ":" above ";
		output << "Warning: The model is " << b.zmin() << where << "the build platform.\n";
	}

	output << "Bounds: ";
	output << "[" << b.xmin() << "," << b.ymin() << "," << b.zmin() << "] ";
	output << "[" << b.xmax() << "," << b.ymax() << "," << b.zmax() << "]\n";
#endif
}

void NodeEvaluator::visit(SubDivisionNode* n)
{
	evaluate(n,Union);
	//TODO
}

void NodeEvaluator::visit(OffsetNode* n)
{
	evaluate(n,Union);
#if USE_CGAL
	result=result->inset(n->getAmount());
#endif
}

void NodeEvaluator::visit(OutlineNode* op)
{
	evaluate(op,Union);

#if USE_CGAL
	CGALExplorer explorer(result);
	QList<CGAL::Point3> points = explorer.getPoints();

	QVector<CGAL::Point3> pl;
	CGAL::Point3 fp;
	for(int i=0; i<points.size(); i++) {
		if(i==0)
			fp=points.at(i);
		pl.append(points.at(i));
	}
	pl.append(fp);

	result=new CGALPrimitive(pl);
#endif
}

void NodeEvaluator::visit(ImportNode* op)
{
#if USE_CGAL
	CGALImport i(output);
	result=i.import(op->getImport());
#endif
}

void NodeEvaluator::visit(TransformationNode* tr)
{
	evaluate(tr,Union);
#if USE_CGAL
	double* m=tr->matrix;
	CGAL::AffTransformation3 t(
		m[0], m[4], m[ 8], m[12],
		m[1], m[5], m[ 9], m[13],
		m[2], m[6], m[10], m[14], m[15]);

	if(result)
		result->transform(t);
#endif
}

void NodeEvaluator::visit(ResizeNode* n)
{
	evaluate(n,Union);
#if USE_CGAL
	CGALExplorer e(result);
	CGAL::Bbox_3 b=e.getBounds();
	Point s=n->getSize();
	double x,y,z;
	s.getXYZ(x,y,z);

	double autosize=1.0;
	if(z!=0.0) {
		z/=(b.zmax()-b.zmin());
		autosize=z;
	}
	if(y!=0.0) {
		y/=(b.ymax()-b.ymin());
		autosize=y;
	}
	if(x!=0.0) {
		x/=(b.xmax()-b.xmin());
		autosize=x;
	}
	if(!n->getAutoSize())
		autosize=1.0;

	if(x==0.0) x=autosize;
	if(y==0.0) y=autosize;
	if(z==0.0) z=autosize;

	CGAL::AffTransformation3 t(
		x, 0, 0, 0,
		0, y, 0, 0,
		0, 0, z, 0, 1);

	result->transform(t);
#endif
}

void NodeEvaluator::visit(CenterNode* n)
{
	evaluate(n,Union);
#if USE_CGAL
	CGALExplorer e(result);
	CGAL::Bbox_3 b=e.getBounds();
	double x,y,z;
	x=(b.xmin()+b.xmax())/2;
	y=(b.ymin()+b.ymax())/2;
	z=(b.zmin()+b.zmax())/2;


	CGAL::AffTransformation3 t(
		1, 0, 0, -x,
		0, 1, 0, -y,
		0, 0, 1, -z, 1);

	result->transform(t);
#endif
}

void NodeEvaluator::visit(PointNode* n)
{
#if USE_CGAL
	QVector<CGAL::Point3> pl1,pl2;
	Point p = n->getPoint();
	double x,y,z;
	p.getXYZ(x,y,z);
	pl1.append(CGAL::Point3(x,y,z));
	pl1.append(CGAL::Point3(x+1,y,z));

	pl2.append(CGAL::Point3(x,y,z));
	pl2.append(CGAL::Point3(x,y+1,z));
	CGALPrimitive* p1 = new CGALPrimitive(pl1);
	CGALPrimitive* p2 = new CGALPrimitive(pl2);

	result = p1->intersection(p2);
#endif
}

void NodeEvaluator::visit(SliceNode* n)
{
	evaluate(n,Union);
#if USE_CGAL
	CGALExplorer e(result);
	CGAL::Bbox_3 b=e.getBounds();

	CGALPrimitive* cp = new CGALPrimitive();
	cp->createPolygon();
	double h = n->getHeight();
	cp->appendVertex(CGAL::Point3(b.xmin(),b.ymin(),h));
	cp->appendVertex(CGAL::Point3(b.xmin(),b.ymax(),h));
	cp->appendVertex(CGAL::Point3(b.xmax(),b.ymax(),h));
	cp->appendVertex(CGAL::Point3(b.xmax(),b.ymin(),h));

	cp->buildVolume();

	result=result->intersection(cp);
#endif
}

Primitive* NodeEvaluator::getResult() const
{
#if USE_CGAL
	return result;
#endif
}
