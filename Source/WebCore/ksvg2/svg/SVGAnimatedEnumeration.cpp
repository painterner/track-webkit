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
#include "SVGStyledElement.h"
#include "SVGAnimatedEnumeration.h"

using namespace WebCore;

SVGAnimatedEnumeration::SVGAnimatedEnumeration(const SVGStyledElement *context) : Shared<SVGAnimatedEnumeration>()
{
    m_baseVal = 0;
    m_animVal = 0;

    m_context = context;
}

SVGAnimatedEnumeration::~SVGAnimatedEnumeration()
{
}

unsigned short SVGAnimatedEnumeration::baseVal() const
{
    return m_baseVal;
}

void SVGAnimatedEnumeration::setBaseVal(unsigned short baseVal)
{
    m_baseVal = baseVal;
    
    // Spec: If the given attribute or property is not currently
    //         being animated, contains the same value as 'baseVal'
    m_animVal = baseVal;

    if(m_context)
        m_context->notifyAttributeChange();
}

unsigned short SVGAnimatedEnumeration::animVal() const
{
    return m_animVal;
}

void SVGAnimatedEnumeration::setAnimVal(unsigned short animVal)
{
    m_animVal = animVal;
    
    if(m_context)
        m_context->notifyAttributeChange();
}

// vim:ts=4:noet
#endif // SVG_SUPPORT

