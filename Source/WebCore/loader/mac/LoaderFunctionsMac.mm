/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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
#import "LoaderFunctions.h"

#import "BlockExceptions.h"
#import "Cache.h"
#import "CachedImage.h"
#import "DocLoader.h"
#import "FoundationExtras.h"
#import "FrameLoader.h"
#import "FrameMac.h"
#import "FormData.h"
#import "FrameLoader.h"
#import "Logging.h"
#import "Request.h"
#import "ResourceHandle.h"
#import "ResourceRequest.h"
#import "loader.h"
#import <wtf/Vector.h>

namespace WebCore {

bool CheckIfReloading(DocLoader *loader)
{
    if (Frame* frame = loader->frame())
        return frame->loader()->isReloading();
    return false;
}

void CheckCacheObjectStatus(DocLoader *loader, CachedResource *cachedResource)
{
    // Return from the function for objects that we didn't load from the cache.
    if (!cachedResource)
        return;
    switch (cachedResource->status()) {
    case CachedResource::Cached:
        break;
    case CachedResource::NotCached:
    case CachedResource::Unknown:
    case CachedResource::New:
    case CachedResource::Pending:
        return;
    }
    
    // Notify the caller that we "loaded".
    FrameMac *frame = static_cast<FrameMac *>(loader->frame());

    if (!frame || frame->loader()->haveToldBridgeAboutLoad(cachedResource->url()))
        return;
        
    NSURLRequest *request = cachedResource->getNSURLRequest();
    const ResourceResponse& response = cachedResource->response();
    RefPtr<SharedBuffer> data = cachedResource->allData();
    
    // FIXME: If the WebKit client changes or cancels the request, WebCore does not respect this and continues the load.
    frame->loader()->loadedResourceFromMemoryCache(request, response, data->size());
    
    frame->loader()->didTellBridgeAboutLoad(cachedResource->url());
}

} // namespace
