/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
                  2005 Alexander Kellett <lypanov@kde.org>

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
#include "SVGMaskElement.h"

#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "RenderSVGContainer.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "cssstyleselector.h"
#include <math.h>
#include <wtf/MathExtras.h>
#include <wtf/OwnPtr.h>

using namespace std;

namespace WebCore {

SVGMaskElement::SVGMaskElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledLocatableElement(tagName, doc)
    , SVGURIReference()
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , m_x(this, LengthModeWidth)
    , m_y(this, LengthModeHeight)
    , m_width(this, LengthModeWidth)
    , m_height(this, LengthModeHeight)
    , m_dirty(true)
{
    // Spec: If the attribute is not specified, the effect is as if a value of "-10%" were specified.
    setXBaseValue(SVGLength(this, LengthModeWidth, "-10%"));
    setYBaseValue(SVGLength(this, LengthModeHeight, "-10%"));
  
    // Spec: If the attribute is not specified, the effect is as if a value of "120%" were specified.
    setWidthBaseValue(SVGLength(this, LengthModeWidth, "120%"));
    setHeightBaseValue(SVGLength(this, LengthModeHeight, "120%"));
}

SVGMaskElement::~SVGMaskElement()
{
}

ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength, Length, length, X, x, SVGNames::xAttr.localName(), m_x)
ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength, Length, length, Y, y, SVGNames::yAttr.localName(), m_y)
ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength, Length, length, Width, width, SVGNames::widthAttr.localName(), m_width)
ANIMATED_PROPERTY_DEFINITIONS(SVGMaskElement, SVGLength, Length, length, Height, height, SVGNames::heightAttr.localName(), m_height)

void SVGMaskElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    IntSize newSize = IntSize(lroundf(width().value()), lroundf(height().value()));
    if (!m_masker || !m_masker->mask() || (m_masker->mask()->size() != newSize))
        m_dirty = true;

    SVGStyledLocatableElement::attributeChanged(attr, preserveDecls);
}

void SVGMaskElement::childrenChanged()
{
    m_dirty = true;
    SVGStyledLocatableElement::childrenChanged();
}

void SVGMaskElement::parseMappedAttribute(MappedAttribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(this, LengthModeWidth, value));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(this, LengthModeHeight, value));
    else if (attr->name() == SVGNames::widthAttr)
        setWidthBaseValue(SVGLength(this, LengthModeWidth, value));
    else if (attr->name() == SVGNames::heightAttr)
        setHeightBaseValue(SVGLength(this, LengthModeHeight, value));
    else {
        if (SVGURIReference::parseMappedAttribute(attr))
            return;
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGStyledElement::parseMappedAttribute(attr);
    }
}

auto_ptr<ImageBuffer> SVGMaskElement::drawMaskerContent()
{
    IntSize size = IntSize(lroundf(width().value()), lroundf(height().value()));

    auto_ptr<ImageBuffer> maskImage = ImageBuffer::create(size, false);
    if (!maskImage.get())
        return maskImage;

    GraphicsContext* maskImageContext = maskImage->context();
    ASSERT(maskImageContext);

    maskImageContext->save();
    maskImageContext->translate(-x().value(), -y().value());

    ImageBuffer::renderSubtreeToImage(maskImage.get(), renderer());

    maskImageContext->restore();
    return maskImage;
}
 
RenderObject* SVGMaskElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    RenderSVGContainer* maskContainer = new (arena) RenderSVGContainer(this);
    maskContainer->setDrawsContents(false);
    return maskContainer;
}

SVGResource* SVGMaskElement::canvasResource()
{
    if (!m_masker) {
        m_masker = new SVGResourceMasker();
        m_dirty = true;
    }
    if (m_dirty) {
        m_masker->setMask(drawMaskerContent());
        m_dirty = !m_masker->mask();
    }
    return m_masker.get();
}

}

#endif // SVG_SUPPORT

// vim:ts=4:noet
