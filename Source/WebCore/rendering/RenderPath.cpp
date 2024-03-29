/*
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Eric Seidel <eric.seidel@kdemail.net>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"

#ifdef SVG_SUPPORT
#include "RenderPath.h"

#include <math.h>

#include "GraphicsContext.h"
#include "RenderSVGContainer.h"
#include "PointerEventsHitRules.h"
#include "SVGPaintServer.h"
#include "SVGResourceClipper.h"
#include "SVGResourceFilter.h"
#include "SVGResourceMasker.h"
#include "SVGResourceMarker.h"
#include "KCanvasRenderingStyle.h"
#include "SVGStyledElement.h"
#include <wtf/MathExtras.h>

namespace WebCore {

// RenderPath
RenderPath::RenderPath(RenderStyle* style, SVGStyledElement* node)
    : RenderObject(node)
{
    ASSERT(style != 0);
}

RenderPath::~RenderPath()
{
}

AffineTransform RenderPath::localTransform() const
{
    return m_matrix;
}

void RenderPath::setLocalTransform(const AffineTransform& matrix)
{
    m_matrix = matrix;
}

FloatPoint RenderPath::mapAbsolutePointToLocal(const FloatPoint& point) const
{
    // FIXME: does it make sense to map incoming points with the inverse of the
    // absolute transform? 
    double localX;
    double localY;
    absoluteTransform().inverse().map(point.x(), point.y(), &localX, &localY);
    return FloatPoint(localX, localY);
}

bool RenderPath::fillContains(const FloatPoint& point, bool requiresFill) const
{
    if (m_path.isEmpty())
        return false;

    if (requiresFill && !KSVGPainterFactory::fillPaintServer(style(), this))
        return false;

    return m_path.contains(point, style()->svgStyle()->fillRule());
}

FloatRect RenderPath::relativeBBox(bool includeStroke) const
{
    if (m_path.isEmpty())
        return FloatRect();

    if (includeStroke) {
        if (m_strokeBbox.isEmpty())
            m_strokeBbox = strokeBBox();

        return m_strokeBbox;
    }

    if (m_fillBBox.isEmpty())
        m_fillBBox = m_path.boundingRect();

    return m_fillBBox;
}

void RenderPath::setPath(const Path& newPath)
{
    m_path = newPath;
    m_strokeBbox = FloatRect();
    m_fillBBox = FloatRect();
}

const Path& RenderPath::path() const
{
    return m_path;
}

void RenderPath::layout()
{
    IntRect oldBounds;
    bool checkForRepaint = checkForRepaintDuringLayout();
    if (selfNeedsLayout() && checkForRepaint)
        oldBounds = m_absoluteBounds;

    // FIXME: Until JSSVGPathSeg* relies on the genericContext<> hack for update
    // notifications, we can't really disable this. It would break js-update-path-changes.svg
    // if (m_path.isEmpty() && m_fillBBox.isEmpty() && m_strokeBbox.isEmpty())
        static_cast<SVGStyledElement*>(element())->rebuildRenderer();

    m_absoluteBounds = getAbsoluteRepaintRect();

    setWidth(m_absoluteBounds.width());
    setHeight(m_absoluteBounds.height());

    if (selfNeedsLayout() && checkForRepaint)
        repaintAfterLayoutIfNeeded(oldBounds, oldBounds);
        
    setNeedsLayout(false);
}

IntRect RenderPath::getAbsoluteRepaintRect()
{
    FloatRect repaintRect = absoluteTransform().mapRect(relativeBBox(true));

    // Filters can expand the bounding box
    SVGResourceFilter* filter = getFilterById(document(), style()->svgStyle()->filter().substring(1));
    if (filter)
        repaintRect.unite(filter->filterBBoxForItemBBox(repaintRect));

    if (!repaintRect.isEmpty())
        repaintRect.inflate(1); // inflate 1 pixel for antialiasing

    return enclosingIntRect(repaintRect);
}

bool RenderPath::requiresLayer()
{
    return false;
}

short RenderPath::lineHeight(bool b, bool isRootLineBox) const
{
    return static_cast<short>(relativeBBox(true).height());
}

short RenderPath::baselinePosition(bool b, bool isRootLineBox) const
{
    return static_cast<short>(relativeBBox(true).height());
}

void RenderPath::paint(PaintInfo& paintInfo, int, int)
{
    if (paintInfo.context->paintingDisabled() || (paintInfo.phase != PaintPhaseForeground) || style()->visibility() == HIDDEN || m_path.isEmpty())
        return;

    paintInfo.context->save();
    paintInfo.context->concatCTM(localTransform());

    // setup to apply filters
    SVGResourceFilter* filter = getFilterById(document(), style()->svgStyle()->filter().substring(1));
    if (filter)
        filter->prepareFilter(paintInfo.context, relativeBBox(true));

    if (SVGResourceClipper* clipper = getClipperById(document(), style()->svgStyle()->clipPath().substring(1)))
        clipper->applyClip(paintInfo.context, relativeBBox(true));

    if (SVGResourceMasker* masker = getMaskerById(document(), style()->svgStyle()->maskElement().substring(1)))
        masker->applyMask(paintInfo.context, relativeBBox(true));

    paintInfo.context->beginPath();
    
    SVGPaintServer* fillPaintServer = KSVGPainterFactory::fillPaintServer(style(), this);
    if (fillPaintServer) {
        paintInfo.context->addPath(m_path);
        fillPaintServer->draw(paintInfo.context, this, ApplyToFillTargetType);
    }

    SVGPaintServer* strokePaintServer = KSVGPainterFactory::strokePaintServer(style(), this);
    if (strokePaintServer) {
        paintInfo.context->addPath(m_path); // path is cleared when filled.
        strokePaintServer->draw(paintInfo.context, this, ApplyToStrokeTargetType);
    }

    drawMarkersIfNeeded(paintInfo.context, paintInfo.rect, m_path);

    // actually apply the filter
    if (filter)
        filter->applyFilter(paintInfo.context, relativeBBox(true));

    paintInfo.context->restore();
}

void RenderPath::absoluteRects(Vector<IntRect>& rects, int _tx, int _ty)
{
    rects.append(getAbsoluteRepaintRect());
}

bool RenderPath::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction)
{
    // We only draw in the forground phase, so we only hit-test then.
    if (hitTestAction != HitTestForeground)
        return false;

    PointerEventsHitRules hitRules(PointerEventsHitRules::SVG_PATH_HITTESTING, style()->svgStyle()->pointerEvents());

    bool isVisible = (style()->visibility() == VISIBLE);
    if (isVisible || !hitRules.requireVisible) {
        FloatPoint hitPoint = mapAbsolutePointToLocal(FloatPoint(_x, _y));
        if ((hitRules.canHitStroke && (style()->svgStyle()->hasStroke() || !hitRules.requireStroke) && strokeContains(hitPoint, hitRules.requireStroke))
            || (hitRules.canHitFill && (style()->svgStyle()->hasFill() || !hitRules.requireFill) && fillContains(hitPoint, hitRules.requireFill))) {
            setInnerNode(result);
            return true;
        }
    }

    return false;
}

enum MarkerType {
    Start,
    Mid,
    End
};

struct MarkerData {
    FloatPoint origin;
    FloatPoint subpathStart;
    double strokeWidth;
    FloatPoint inslopePoints[2];
    FloatPoint outslopePoints[2];
    MarkerType type;
    SVGResourceMarker *marker;
};

struct DrawMarkersData {
    DrawMarkersData(GraphicsContext*, SVGResourceMarker* startMarker, SVGResourceMarker* midMarker, double strokeWidth);
    GraphicsContext* context;
    int elementIndex;
    MarkerData previousMarkerData;
    SVGResourceMarker* midMarker;
};

DrawMarkersData::DrawMarkersData(GraphicsContext* c, SVGResourceMarker *start, SVGResourceMarker *mid, double strokeWidth)
    : context(c)
    , elementIndex(0)
    , midMarker(mid)
{
    previousMarkerData.origin = FloatPoint();
    previousMarkerData.subpathStart = FloatPoint();
    previousMarkerData.strokeWidth = strokeWidth;
    previousMarkerData.marker = start;
    previousMarkerData.type = Start;
}

static void drawMarkerWithData(GraphicsContext* context, MarkerData &data)
{
    if (!data.marker)
        return;
    
    FloatPoint inslopeChange = data.inslopePoints[1] - FloatSize(data.inslopePoints[0].x(), data.inslopePoints[0].y());
    FloatPoint outslopeChange = data.outslopePoints[1] - FloatSize(data.outslopePoints[0].x(), data.outslopePoints[0].y());
    
    static const double deg2rad = M_PI / 180.0;
    double inslope = atan2(inslopeChange.y(), inslopeChange.x()) / deg2rad;
    double outslope = atan2(outslopeChange.y(), outslopeChange.x()) / deg2rad;
    
    double angle = 0.0;
    switch (data.type) {
        case Start:
            angle = outslope;
            break;
        case Mid:
            angle = (inslope + outslope) / 2;
            break;
        case End:
            angle = inslope;
    }
    
    data.marker->draw(context, FloatRect(), data.origin.x(), data.origin.y(), data.strokeWidth, angle);
}

static inline void updateMarkerDataForElement(MarkerData& previousMarkerData, const PathElement* element)
{
    FloatPoint* points = element->points;
    
    switch (element->type) {
    case PathElementAddQuadCurveToPoint:
        // TODO
        previousMarkerData.origin = points[1];
        break;
    case PathElementAddCurveToPoint:
        previousMarkerData.inslopePoints[0] = points[1];
        previousMarkerData.inslopePoints[1] = points[2];
        previousMarkerData.origin = points[2];
        break;
    case PathElementMoveToPoint:
        previousMarkerData.subpathStart = points[0];
    case PathElementAddLineToPoint:
        previousMarkerData.inslopePoints[0] = previousMarkerData.origin;
        previousMarkerData.inslopePoints[1] = points[0];
        previousMarkerData.origin = points[0];
        break;
    case PathElementCloseSubpath:
        previousMarkerData.inslopePoints[0] = previousMarkerData.origin;
        previousMarkerData.inslopePoints[1] = points[0];
        previousMarkerData.origin = previousMarkerData.subpathStart;
        previousMarkerData.subpathStart = FloatPoint();
    }
}

static void drawStartAndMidMarkers(void* info, const PathElement* element)
{
    DrawMarkersData& data = *reinterpret_cast<DrawMarkersData*>(info);

    int elementIndex = data.elementIndex;
    MarkerData& previousMarkerData = data.previousMarkerData;

    FloatPoint* points = element->points;

    // First update the outslope for the previous element
    previousMarkerData.outslopePoints[0] = previousMarkerData.origin;
    previousMarkerData.outslopePoints[1] = points[0];

    // Draw the marker for the previous element
    if (elementIndex != 0)
        drawMarkerWithData(data.context, previousMarkerData);

    // Update our marker data for this element
    updateMarkerDataForElement(previousMarkerData, element);

    if (elementIndex == 1) {
        // After drawing the start marker, switch to drawing mid markers
        previousMarkerData.marker = data.midMarker;
        previousMarkerData.type = Mid;
    }

    data.elementIndex++;
}

void RenderPath::drawMarkersIfNeeded(GraphicsContext* context, const FloatRect& rect, const Path& path) const
{
    Document* doc = document();
    const SVGRenderStyle* svgStyle = style()->svgStyle();

    SVGResourceMarker* startMarker = getMarkerById(doc, svgStyle->startMarker().substring(1));
    SVGResourceMarker* midMarker = getMarkerById(doc, svgStyle->midMarker().substring(1));
    SVGResourceMarker* endMarker = getMarkerById(doc, svgStyle->endMarker().substring(1));
    
    if (!startMarker && !midMarker && !endMarker)
        return;

    double strokeWidth = KSVGPainterFactory::cssPrimitiveToLength(this, style()->svgStyle()->strokeWidth(), 1.0);
    DrawMarkersData data(context, startMarker, midMarker, strokeWidth);

    path.apply(&data, drawStartAndMidMarkers);

    data.previousMarkerData.marker = endMarker;
    data.previousMarkerData.type = End;
    drawMarkerWithData(context, data.previousMarkerData);
}

bool RenderPath::hasRelativeValues() const
{
    return static_cast<SVGStyledElement*>(element())->hasRelativeValues();
}
 
}

#endif // SVG_SUPPORT
