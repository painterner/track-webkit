/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
                  2006 Alexander Kellett <lypanov@kde.org>

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
#include "SVGImageElement.h"

#include "CSSPropertyNames.h"
#include "RenderSVGContainer.h"
#include "RenderSVGImage.h"
#include "SVGDocument.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGSVGElement.h"
#include "XLinkNames.h"

namespace WebCore {

SVGImageElement::SVGImageElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledTransformableElement(tagName, doc)
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , SVGURIReference()
    , m_x(this, LengthModeWidth)
    , m_y(this, LengthModeHeight)
    , m_width(this, LengthModeWidth)
    , m_height(this, LengthModeHeight)
    , m_preserveAspectRatio(new SVGPreserveAspectRatio(this))
    , m_imageLoader(this)
{
}

SVGImageElement::~SVGImageElement()
{
}

ANIMATED_PROPERTY_DEFINITIONS(SVGImageElement, SVGLength, Length, length, X, x, SVGNames::xAttr.localName(), m_x)
ANIMATED_PROPERTY_DEFINITIONS(SVGImageElement, SVGLength, Length, length, Y, y, SVGNames::yAttr.localName(), m_y)
ANIMATED_PROPERTY_DEFINITIONS(SVGImageElement, SVGLength, Length, length, Width, width, SVGNames::widthAttr.localName(), m_width)
ANIMATED_PROPERTY_DEFINITIONS(SVGImageElement, SVGLength, Length, length, Height, height, SVGNames::heightAttr.localName(), m_height)
ANIMATED_PROPERTY_DEFINITIONS(SVGImageElement, SVGPreserveAspectRatio*, PreserveAspectRatio, preserveAspectRatio, PreserveAspectRatio, preserveAspectRatio, SVGNames::preserveAspectRatioAttr.localName(), m_preserveAspectRatio.get())

void SVGImageElement::parseMappedAttribute(MappedAttribute *attr)
{
    const AtomicString& value = attr->value();
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(this, LengthModeWidth, value));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(this, LengthModeHeight, value));
    else if (attr->name() == SVGNames::preserveAspectRatioAttr)
        preserveAspectRatioBaseValue()->parsePreserveAspectRatio(value);
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(this, LengthModeWidth, value));
        addCSSProperty(attr, CSS_PROP_WIDTH, value);
        if (width().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for image attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(this, LengthModeHeight, value));
        addCSSProperty(attr, CSS_PROP_HEIGHT, value);
        if (height().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for image attribute <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGURIReference::parseMappedAttribute(attr)) {
            if (attr->name().matches(XLinkNames::hrefAttr))
                m_imageLoader.updateFromElement();
            return;
        }
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

void SVGImageElement::notifyAttributeChange() const
{
    if (!ownerDocument()->parsing())
        rebuildRenderer();
}

RenderObject* SVGImageElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    return new (arena) RenderSVGImage(this);
}

bool SVGImageElement::haveLoadedRequiredResources()
{
    return (!externalResourcesRequiredBaseValue() || m_imageLoader.imageComplete());
}

void SVGImageElement::attach()
{
    SVGStyledTransformableElement::attach();
    if (RenderSVGImage* imageObj = static_cast<RenderSVGImage*>(renderer()))
        imageObj->setCachedImage(m_imageLoader.image());
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet
