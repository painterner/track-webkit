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

#ifndef KSVG_SVGAnimatedLengthImpl_H
#define KSVG_SVGAnimatedLengthImpl_H
#ifdef SVG_SUPPORT

#include "SVGLength.h"
#include "SVGAnimatedTemplate.h"

namespace WebCore
{
    class SVGAnimatedLength : public SVGAnimatedTemplate<SVGLength>
    {
    public:
        SVGAnimatedLength(const SVGStyledElement *context, LengthMode mode = LM_UNKNOWN, const SVGElement *viewport = 0);
        virtual ~SVGAnimatedLength();

    protected:
        virtual SVGLength *create() const;
        virtual void assign(SVGLength *src, SVGLength *dst) const;

    private:
        LengthMode m_mode;
        const SVGElement *m_viewportElement;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet