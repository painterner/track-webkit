/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
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

#ifndef KSVG_SVGTitleElementImpl_H
#define KSVG_SVGTitleElementImpl_H
#ifdef SVG_SUPPORT

#include "SVGLangSpace.h"
#include "SVGStyledElement.h"

namespace WebCore
{
    class SVGTitleElement : public SVGStyledElement,
                            public SVGLangSpace
    {
    public:
        SVGTitleElement(const QualifiedName&, Document*);

        String title() const;

        virtual void insertedIntoDocument();
        virtual void removedFromDocument();
        virtual void childrenChanged();
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
