/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef RenderSVGText_H
#define RenderSVGText_H
#ifdef SVG_SUPPORT

#include "RenderBlock.h"
#include "AffineTransform.h"

namespace WebCore {

class SVGTextElement;

class RenderSVGText : public RenderBlock
{
public:
    RenderSVGText(SVGTextElement *node);

    virtual const char *renderName() const { return "RenderSVGText"; }
    virtual void paint(PaintInfo&, int parentX, int parentY);
    
    virtual AffineTransform localTransform() const { return m_transform; }
    virtual void setLocalTransform(const AffineTransform& transform) { m_transform = transform; }
    
    virtual bool nodeAtPoint(NodeInfo&, int _x, int _y, int _tx, int _ty, HitTestAction);

    virtual void absoluteRects(DeprecatedValueList<IntRect>& rects, int tx, int ty);

    virtual void computeAbsoluteRepaintRect(IntRect& r, bool f);
    virtual bool requiresLayer();
    virtual void layout();
    
 private:
    AffineTransform translationTopToBaseline();
    AffineTransform translationForAttributes();
    AffineTransform m_transform;
    IntRect m_absoluteBounds;
};

}

#endif // SVG_SUPPORT
#endif
