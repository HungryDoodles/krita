/*
 * Copyright (c) 2008 Cyrille Berger <cberger@cberger.net>
 * Copyright (c) 2010 Geoffry Song <goffrie@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _CONCENTRIC_ELLIPSE_ASSISTANT_H_
#define _CONCENTRIC_ELLIPSE_ASSISTANT_H_

#include "kis_painting_assistant.h"
#include "Ellipse.h"
#include <QLineF>
#include <QObject>

class ConcentricEllipseAssistant : public KisPaintingAssistant
{
public:
    ConcentricEllipseAssistant();
    virtual QPointF adjustPosition(const QPointF& point, const QPointF& strokeBegin);
    virtual QPointF buttonPosition() const;
    virtual int numHandles() const { return 3; }
protected:
    virtual QRect boundingRect() const;
    virtual void drawAssistant(QPainter& gc, const QRectF& updateRect, const KisCoordinatesConverter* converter, bool cached, KisCanvas2* canvas, bool assistantVisible=true, bool previewVisible=true);
    virtual void drawCache(QPainter& gc, const KisCoordinatesConverter *converter,  bool assistantVisible=true);
private:
    QPointF project(const QPointF& pt, const QPointF& strokeBegin) const;
    mutable Ellipse e;
    mutable Ellipse extraE;
};

class ConcentricEllipseAssistantFactory : public KisPaintingAssistantFactory
{
public:
    ConcentricEllipseAssistantFactory();
    virtual ~ConcentricEllipseAssistantFactory();
    virtual QString id() const;
    virtual QString name() const;
    virtual KisPaintingAssistant* createPaintingAssistant() const;
};

#endif
