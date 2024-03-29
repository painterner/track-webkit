/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Jonas Witt <jonas.witt@gmail.com>
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
#import "DOMPrivate.h"

#import "DOMEventsInternal.h"
#import "DOMViewsInternal.h"
#import "DOMInternal.h"
#import "KeyboardEvent.h"
#import "WheelEvent.h"

using namespace WebCore;

ALLOW_DOM_CAST(Event)

@implementation DOMWheelEvent

- (WheelEvent *)_wheelEvent
{
    return static_cast<WheelEvent *>(DOM_cast<Event *>(_internal));
}

- (int)screenX
{
    return [self _wheelEvent]->screenX();
}

- (int)screenY
{
    return [self _wheelEvent]->screenY();
}

- (int)clientX
{
    return [self _wheelEvent]->clientX();
}

- (int)clientY
{
    return [self _wheelEvent]->clientY();
}

- (BOOL)ctrlKey
{
    return [self _wheelEvent]->ctrlKey();
}

- (BOOL)shiftKey
{
    return [self _wheelEvent]->shiftKey();
}

- (BOOL)altKey {
    return [self _wheelEvent]->altKey();
}

- (BOOL)metaKey {
    return [self _wheelEvent]->metaKey();
}

- (BOOL)isHorizontal
{
    return [self _wheelEvent]->isHorizontal();
}

- (int)wheelDelta
{
    return [self _wheelEvent]->wheelDelta();
}

- (void)initWheelEvent:(BOOL)horizontal :(int)wheelDelta :(DOMAbstractView *)viewArg :(int)screenXArg :(int)screenYArg :(int)clientX :(int)clientY :(BOOL)ctrlKeyArg :(BOOL)altKeyArg :(BOOL)shiftKeyArg :(BOOL)metaKeyArg
{
    [self _wheelEvent]->initWheelEvent(horizontal, wheelDelta, [viewArg _abstractView], 
        screenXArg, screenYArg, clientX, clientY,
        ctrlKeyArg, altKeyArg, shiftKeyArg, metaKeyArg);
}

@end

@implementation DOMKeyboardEvent (NonStandardAdditions)

- (KeyboardEvent *)_keyboardEvent
{
    return static_cast<KeyboardEvent *>(DOM_cast<Event *>(_internal));
}

- (int)keyCode
{
    return [self _keyboardEvent]->keyCode();
}

- (int)charCode
{
    return [self _keyboardEvent]->charCode();
}

@end
