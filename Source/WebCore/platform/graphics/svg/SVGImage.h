/*
 * Copyright (C) 2006 Eric Seidel (eric@webkit.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SVGImage_H
#define SVGImage_H

#ifdef SVG_SUPPORT

#include "Image.h"
#include "IntSize.h"
#include <wtf/OwnPtr.h>

namespace WebCore {
    
    class SVGDocument;
    class Frame;
    class FrameView;
    class Page;
    
    class SVGImage : public Image {
    public:
        SVGImage(ImageAnimationObserver*);
        ~SVGImage();
        
        virtual IntSize size() const;
        
        virtual bool setData(bool allDataReceived);
        
        virtual NativeImagePtr frameAtIndex(size_t) { return 0; }
        
private:
        virtual void draw(GraphicsContext*, const FloatRect& fromRect, const FloatRect& toRect, CompositeOperator);
        virtual void drawTiled(GraphicsContext*, const FloatRect& dstRect, const FloatPoint& srcPoint, const FloatSize& tileSize, CompositeOperator);
        virtual void drawTiled(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, TileRule hRule, TileRule vRule, CompositeOperator);
        
        SVGDocument* m_document;
        OwnPtr<Page> m_page;
        RefPtr<Frame> m_frame;
        RefPtr<FrameView> m_frameView;
        IntSize m_minSize;
    };
}

#endif // SVG_SUPPORT

#endif
