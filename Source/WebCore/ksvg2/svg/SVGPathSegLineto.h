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

#ifndef SVGPathSegLineto_H
#define SVGPathSegLineto_H

#ifdef SVG_SUPPORT

#include "SVGPathSeg.h"

namespace WebCore
{
    class SVGPathSegLinetoAbs : public SVGPathSeg
    { 
    public:
        SVGPathSegLinetoAbs(double x, double y);
        virtual ~SVGPathSegLinetoAbs();

        virtual unsigned short pathSegType() const { return PATHSEG_LINETO_ABS; }
        virtual String pathSegTypeAsLetter() const { return "L"; }
        virtual String toString() const { return String::format("L %.6lg %.6lg", m_x, m_y); }

        void setX(double);
        double x() const;

        void setY(double);
        double y() const;

    private:
        double m_x;
        double m_y;
    };

    class SVGPathSegLinetoRel : public SVGPathSeg
    { 
    public:
        SVGPathSegLinetoRel(double x, double y);
        virtual ~SVGPathSegLinetoRel();

        virtual unsigned short pathSegType() const { return PATHSEG_LINETO_REL; }
        virtual String pathSegTypeAsLetter() const { return "l"; }
        virtual String toString() const { return String::format("l %.6lg %.6lg", m_x, m_y); }

        void setX(double);
        double x() const;

        void setY(double);
        double y() const;

    private:
        double m_x;
        double m_y;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
