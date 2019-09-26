/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "ImageDocumentMac.h"

#include "CachedImage.h"
#include "Document.h"
#include "FrameLoader.h"
#include "FrameMac.h"
#include "WebCoreFrameBridge.h"
#include "DocumentLoader.h"

namespace WebCore {
    
void finishImageLoad(Document* document, CachedImage* image, const void* imageData, size_t imageDataSize)
{
    // FIXME: This is terrible! Makes an extra copy of the image data!
    // Can't we get the NSData from NSURLConnection?
    // Why is this different from image subresources?
    RefPtr<SharedBuffer> buffer = new SharedBuffer(reinterpret_cast<const char*>(imageData), imageDataSize);

    Frame* frame = document->frame();
    const ResourceResponse& response = frame->loader()->documentLoader()->response();

    IntSize size = image->imageSize();
    if (size.width())
        document->setTitle([Mac(frame)->bridge() imageTitleForFilename:response.suggestedFilename() size:size]);
}
    
}