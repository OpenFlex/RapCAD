/*
 *   RapCAD - Rapid prototyping CAD IDE (www.rapcad.org)
 *   Copyright (C) 2010-2011 Giles Bathgate
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

#ifndef GLVIEW_H
#define GLVIEW_H

#include <QGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include "renderer.h"

class GLView : public QGLWidget
{
public:
	GLView(QWidget* parent = NULL);
	void setRenderer(Renderer* r);
private:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void wheelEvent(QWheelEvent *event);

	void NormalizeAngle(double&);

	Renderer* render;
	double distance;
	bool showaxes;
	bool mouseDrag;
	QPoint last;
	double rotateX;
	double rotateY;
	double rotateZ;
};

#endif // GLVIEW_H
