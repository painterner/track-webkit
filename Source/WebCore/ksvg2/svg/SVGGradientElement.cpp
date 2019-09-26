/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>

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
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"

#ifdef SVG_SUPPORT
#include "SVGGradientElement.h"

#include "cssstyleselector.h"
#include "RenderPath.h"
#include "RenderView.h"
#include "SVGNames.h"
#include "SVGStopElement.h"
#include "SVGTransformList.h"
#include "SVGTransformable.h"
#include "SVGUnitTypes.h"
#include "SVGPaintServerLinearGradient.h"
#include "SVGPaintServerRadialGradient.h"

namespace WebCore {

SVGGradientElement::SVGGradientElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledElement(tagName, doc)
    , SVGURIReference()
    , SVGExternalResourcesRequired()
    , m_spreadMethod(0)
    , m_gradientUnits(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX)
    , m_gradientTransform(new SVGTransformList)
{
}

SVGGradientElement::~SVGGradientElement()
{
}

ANIMATED_PROPERTY_DEFINITIONS(SVGGradientElement, int, Enumeration, enumeration, GradientUnits, gradientUnits, SVGNames::gradientUnitsAttr.localName(), m_gradientUnits)
ANIMATED_PROPERTY_DEFINITIONS(SVGGradientElement, SVGTransformList*, TransformList, transformList, GradientTransform, gradientTransform, SVGNames::gradientTransformAttr.localName(), m_gradientTransform.get())
ANIMATED_PROPERTY_DEFINITIONS(SVGGradientElement, int, Enumeration, enumeration, SpreadMethod, spreadMethod, SVGNames::spreadMethodAttr.localName(), m_spreadMethod)

void SVGGradientElement::parseMappedAttribute(MappedAttribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::gradientUnitsAttr) {
        if (value == "userSpaceOnUse")
            setGradientUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE);
        else if (value == "objectBoundingBox")
            setGradientUnitsBaseValue(SVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
    } else if (attr->name() == SVGNames::gradientTransformAttr) {
        SVGTransformList* gradientTransforms = gradientTransformBaseValue();
        if (!SVGTransformable::parseTransformAttribute(gradientTransforms, attr->value())) {
            ExceptionCode ec = 0;
            gradientTransforms->clear(ec);
        }
    } else if (attr->name() == SVGNames::spreadMethodAttr) {
        if (value == "reflect")
            setSpreadMethodBaseValue(SVG_SPREADMETHOD_REFLECT);
        else if (value == "repeat")
            setSpreadMethodBaseValue(SVG_SPREADMETHOD_REPEAT);
        else if (value == "pad")
            setSpreadMethodBaseValue(SVG_SPREADMETHOD_PAD);
    } else {
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        
        SVGStyledElement::parseMappedAttribute(attr);
    }
}

void SVGGradientElement::notifyAttributeChange() const
{
    if (!m_resource || !attached() || ownerDocument()->parsing())
        return;

    m_resource->invalidate();
    m_resource->repaintClients();
}

SVGResource* SVGGradientElement::canvasResource()
{
    if (!m_resource) {
        if (gradientType() == LinearGradientPaintServer)
            m_resource = new SVGPaintServerLinearGradient(this);
        else
            m_resource = new SVGPaintServerRadialGradient(this);
    }

    return m_resource.get();
}

Vector<SVGGradientStop> SVGGradientElement::buildStops() const
{
    Vector<SVGGradientStop> stops;

    // FIXME: Manual style resolution is a hack
    RenderStyle* gradientStyle = const_cast<SVGGradientElement*>(this)->styleForRenderer(parent()->renderer());
    for (Node* n = firstChild(); n; n = n->nextSibling()) {
        SVGElement* element = svg_dynamic_cast(n);
        if (element && element->isGradientStop()) {
            SVGStopElement* stop = static_cast<SVGStopElement*>(element);
            float stopOffset = stop->offset();
                
            RenderStyle* stopStyle = document()->styleSelector()->styleForElement(stop, gradientStyle);
            Color c = stopStyle->svgStyle()->stopColor();
            float opacity = stopStyle->svgStyle()->stopOpacity();
                
            stops.append(makeGradientStop(stopOffset, makeRGBA(c.red(), c.green(), c.blue(), int(opacity * 255.))));
            stopStyle->deref(view()->renderArena());
        }
    }

    gradientStyle->deref(view()->renderArena());
    return stops;
}

void SVGGradientElement::insertedIntoDocument()
{
    SVGElement::insertedIntoDocument();
    SVGDocumentExtensions* extensions = document()->accessSVGExtensions();

    String resourceId = SVGURIReference::getTarget(id());
    if (extensions->isPendingResource(resourceId))
        SVGResource::repaintClients(extensions->removePendingResource(resourceId));
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet