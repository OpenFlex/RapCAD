/*
 *   RapCAD - Rapid prototyping CAD IDE (www.rapcad.org)
 *   Copyright (C) 2010-2013 Giles Bathgate
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

#include "polyhedronmodule.h"
#include "context.h"
#include "vectorvalue.h"
#include "numbervalue.h"
#include "node/primitivenode.h"

PolyhedronModule::PolyhedronModule() : PrimitiveModule("polyhedron")
{
	addParameter("points");
	addParameter("surfaces");
}

Node* PolyhedronModule::evaluate(Context* ctx)
{
	VectorValue* points=dynamic_cast<VectorValue*>(getParameterArgument(ctx,0));
	VectorValue* surfaces=dynamic_cast<VectorValue*>(ctx->getArgumentDeprecated(1,"surfaces","triangles"));

	QList<Value*> children = points->getChildren();

	PrimitiveNode* p=new PrimitiveNode();
	foreach(Value* s,surfaces->getChildren()) {
		p->createPolygon();
		VectorValue* surface=dynamic_cast<VectorValue*>(s);
		foreach(Value* indexVal,surface->getChildren()) {
			NumberValue* indexNum=dynamic_cast<NumberValue*>(indexVal);
			double index = indexNum->getNumber();
			VectorValue* point=dynamic_cast<VectorValue*>(children.at(index));
			Point pt = point->getPoint();
			p->appendVertex(pt);
		}

	}

	return p;

}
