/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

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
#include "SVGAnimatedLength.h"

using namespace WebCore;

SVGAnimatedLength::SVGAnimatedLength(const SVGStyledElement *context, LengthMode mode, const SVGElement *viewport) : SVGAnimatedTemplate<SVGLength>(context)
{
    m_mode = mode;
    m_viewportElement = viewport;
}

SVGAnimatedLength::~SVGAnimatedLength()
{
}

SVGLength *SVGAnimatedLength::create() const
{
    return new SVGLength(m_context, m_mode, m_viewportElement);
}

void SVGAnimatedLength::assign(SVGLength *src, SVGLength *dst) const
{
    if(!src || !dst)
        return;
    
    dst->setValueAsString(src->valueAsString());
}

// vim:ts=4:noet
#endif // SVG_SUPPORT

