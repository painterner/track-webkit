/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#import "config.h"
#import "ScrollView.h"

#import "FloatRect.h"
#import "IntRect.h"
#import "BlockExceptions.h"
#import "Logging.h"
#import "WebCoreFrameView.h"

/*
    This class implementation does NOT actually emulate the Qt ScrollView.
    It does provide an implementation that khtml will use to interact with
    WebKit's WebFrameView documentView and our NSScrollView subclass.

    ScrollView's view is a NSScrollView (or subclass of NSScrollView)
    in most cases. That scrollview is a subview of an
    WebCoreFrameView. The WebCoreFrameView's documentView will also be
    the scroll view's documentView.
    
    The WebCoreFrameView's size is the frame size.  The WebCoreFrameView's documentView
    corresponds to the frame content size.  The scrollview itself is autosized to the
    WebCoreFrameView's size (see Widget::resize).
*/

namespace WebCore {

int ScrollView::visibleWidth() const
{
    NSScrollView *view = (NSScrollView *)getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view isKindOfClass:[NSScrollView class]])
        return (int)[view documentVisibleRect].size.width;
    else
        return (int)[view bounds].size.width;
    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

int ScrollView::visibleHeight() const
{
    NSScrollView *view = (NSScrollView *)getView();
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view isKindOfClass:[NSScrollView class]])
        return (int)[view documentVisibleRect].size.height;
    else
        return (int)[view bounds].size.height;
    END_BLOCK_OBJC_EXCEPTIONS;
    
    return 0;
}

FloatRect ScrollView::visibleContentRect() const
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if (NSView *docView = getDocumentView())
        return [docView visibleRect];
    END_BLOCK_OBJC_EXCEPTIONS;
    return FloatRect();
}

int ScrollView::contentsWidth() const
{
    NSView *docView, *view = getView();
    docView = getDocumentView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if (docView)
        return (int)[docView bounds].size.width;
    else
        return (int)[view bounds].size.width;
    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

int ScrollView::contentsHeight() const
{
    NSView *docView, *view = getView();
    docView = getDocumentView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if (docView)
        return (int)[docView bounds].size.height;
    else
        return (int)[view bounds].size.height;
    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

int ScrollView::contentsX() const
{
    NSView *view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view isKindOfClass:[NSScrollView class]])
        return (int)[(NSScrollView *)view documentVisibleRect].origin.x;
    else
        return (int)[view visibleRect].origin.x;
    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

int ScrollView::contentsY() const
{
    NSView *view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view isKindOfClass:[NSScrollView class]])
        return (int)[(NSScrollView *)view documentVisibleRect].origin.y;
    else
        return (int)[view visibleRect].origin.y;
    END_BLOCK_OBJC_EXCEPTIONS;

    return 0;
}

IntSize ScrollView::scrollOffset() const
{
    NSView *view = getView();
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view isKindOfClass:[NSScrollView class]])
        return IntPoint([[(NSScrollView *)view contentView] visibleRect].origin) - IntPoint();
    END_BLOCK_OBJC_EXCEPTIONS;
    return IntSize();
}

void ScrollView::scrollBy(int dx, int dy)
{
    setContentsPos(contentsX() + dx, contentsY() + dy);
}

void ScrollView::scrollPointRecursively(int x, int y)
{ 
    x = (x < 0) ? 0 : x;
    y = (y < 0) ? 0 : y;
    NSPoint p = NSMakePoint(x,y);
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSView *docView;
    NSView *view = getView();    
    docView = getDocumentView();
    if (docView)
        view = docView;
    
    NSView *originalView = view;
    while (view) {
        if ([view isKindOfClass:[NSClipView class]]) {
            NSPoint viewPoint = [view convertPoint:p fromView:originalView];
            [view scrollPoint:viewPoint];
        }
        view = [view superview];
    }
    END_BLOCK_OBJC_EXCEPTIONS;
}

void ScrollView::setContentsPos(int x, int y)
{
    x = (x < 0) ? 0 : x;
    y = (y < 0) ? 0 : y;
    NSPoint p =  NSMakePoint(x,y);

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSView *docView;
    NSView *view = getView();    
    docView = getDocumentView();
    if (docView)
        view = docView;
    [view scrollPoint:p];
    END_BLOCK_OBJC_EXCEPTIONS;
}

void ScrollView::setVScrollBarMode(ScrollBarMode vMode)
{
    NSView* view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view conformsToProtocol:@protocol(WebCoreFrameView)]) {
        NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
        [frameView setVerticalScrollingMode: (WebCoreScrollBarMode)vMode];
    }
    END_BLOCK_OBJC_EXCEPTIONS;
}

void ScrollView::setHScrollBarMode(ScrollBarMode hMode)
{
    NSView* view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view conformsToProtocol:@protocol(WebCoreFrameView)]) {
        NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
        [frameView setHorizontalScrollingMode: (WebCoreScrollBarMode)hMode];
    }
    END_BLOCK_OBJC_EXCEPTIONS;
}

void ScrollView::setScrollBarsMode(ScrollBarMode mode)
{
    NSView* view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view conformsToProtocol:@protocol(WebCoreFrameView)]) {
        NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
        [frameView setScrollingMode: (WebCoreScrollBarMode)mode];
    }
    END_BLOCK_OBJC_EXCEPTIONS;
}

ScrollBarMode ScrollView::vScrollBarMode() const
{
    NSView* view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view conformsToProtocol:@protocol(WebCoreFrameView)]) {
        NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
        return (ScrollBarMode)[frameView verticalScrollingMode];
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return ScrollBarAuto;
}

ScrollBarMode ScrollView::hScrollBarMode() const
{
    NSView* view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view conformsToProtocol:@protocol(WebCoreFrameView)]) {
        NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
        return (ScrollBarMode)[frameView horizontalScrollingMode];
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return ScrollBarAuto;
}

void ScrollView::suppressScrollBars(bool suppressed,  bool repaintOnUnsuppress)
{
    NSView* view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view conformsToProtocol:@protocol(WebCoreFrameView)]) {
        NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
        [frameView setScrollBarsSuppressed: suppressed
                       repaintOnUnsuppress: repaintOnUnsuppress];
    }
    END_BLOCK_OBJC_EXCEPTIONS;
}

void ScrollView::addChild(Widget* child, int x, int y)
{
    ASSERT(child != this);
    
    // we don't need to do the offscreen position initialization that KDE needs
    if (x != -500000)
        child->move(x, y);

    NSView *thisView = getView();
    NSView *thisDocView = getDocumentView();
    if (thisDocView)
        thisView = thisDocView;

#ifndef NDEBUG
    NSView *subview = child->getOuterView();
    
    LOG(Frames, "Adding %p %@ at (%d,%d) w %d h %d\n", subview,
        [(id)[subview class] className], x, y, (int)[subview frame].size.width, (int)[subview frame].size.height);
#endif
    child->addToSuperview(thisView);
}

void ScrollView::removeChild(Widget* child)
{
    child->removeFromSuperview();
}

void ScrollView::resizeContents(int w, int h)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    int _w = w;
    int _h = h;

    LOG(Frames, "%p %@ at w %d h %d\n", getView(), [(id)[getView() class] className], w, h);
    NSView *view = getView();
    if ([view isKindOfClass:[NSScrollView class]]){
        view = getDocumentView();
        
        LOG(Frames, "%p %@ at w %d h %d\n", view, [(id)[view class] className], w, h);
        if (_w < 0)
            _w = 0;
        if (_h < 0)
            _h = 0;
            
        NSSize tempSize = { _w, _h }; // workaround for 4213314
        [view setFrameSize:tempSize];
    } else {
        resize (_w, _h);
    }
    END_BLOCK_OBJC_EXCEPTIONS;
}

void ScrollView::updateContents(const IntRect &rect, bool now)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView *view = getView();

    if ([view isKindOfClass:[NSScrollView class]])
        view = getDocumentView();

    // Checking for rect visibility is an important optimization for the case of
    // Select All of a large document. AppKit does not do this check, and so ends
    // up building a large complicated NSRegion if we don't perform the check.
    NSRect dirtyRect = NSIntersectionRect(rect, [view visibleRect]);
    if (!NSIsEmptyRect(dirtyRect)) {
        [view setNeedsDisplayInRect:dirtyRect];
        if (now) {
            [[view window] displayIfNeeded];
            [[view window] flushWindowIfNeeded];
        }
    }

    END_BLOCK_OBJC_EXCEPTIONS;
}

// NB, for us "viewport" means the NSWindow's coord system, which is origin lower left

IntPoint ScrollView::contentsToViewport(const IntPoint& contentsPoint)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView *docView;
    NSView *view = getView();    
     
    docView = getDocumentView();
    if (docView)
        view = docView;
    
    NSPoint tempPoint = { contentsPoint.x(), contentsPoint.y() }; // workaround for 4213314
    NSPoint np = [view convertPoint:tempPoint toView: nil];
    return IntPoint(np);

    END_BLOCK_OBJC_EXCEPTIONS;
    
    return IntPoint();
}

IntPoint ScrollView::viewportToContents(const IntPoint& viewportPoint)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView *docView;
    NSView *view = getView();    

    docView = getDocumentView();
    if (docView)
        view = docView;
    
    NSPoint tempPoint = { viewportPoint.x(), viewportPoint.y() }; // workaround for 4213314
    NSPoint np = [view convertPoint:tempPoint fromView: nil];

    return IntPoint(np);

    END_BLOCK_OBJC_EXCEPTIONS;

    return IntPoint();
}

void ScrollView::setStaticBackground(bool b)
{
    NSScrollView *view = (NSScrollView *)getView();
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view isKindOfClass:[NSScrollView class]])
        [[view contentView] setCopiesOnScroll: !b];
    END_BLOCK_OBJC_EXCEPTIONS;
}

NSView *ScrollView::getDocumentView() const
{
    id view = getView();

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    if ([view respondsToSelector:@selector(documentView)]) 
        return [view documentView];
    END_BLOCK_OBJC_EXCEPTIONS;
    
    return nil;
}

bool ScrollView::inWindow() const
{
    return [getView() window];
}

}