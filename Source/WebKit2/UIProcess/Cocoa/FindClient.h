/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FindClient_h
#define FindClient_h

#import "WKFoundation.h"

#if WK_API_ENABLED

#import "APIFindClient.h"
#import "WeakObjCPtr.h"

@class WKWebView;
@protocol _WKFindDelegate;

namespace WebKit {

class FindClient final : public API::FindClient {
public:
    explicit FindClient(WKWebView *);
    
    RetainPtr<id <_WKFindDelegate>> delegate();
    void setDelegate(id <_WKFindDelegate>);
    
private:
    // From API::FindClient
    virtual void didCountStringMatches(WebPageProxy*, const String&, uint32_t matchCount);
    virtual void didFindString(WebPageProxy*, const String&, uint32_t matchCount, int32_t matchIndex);
    virtual void didFailToFindString(WebPageProxy*, const String&);
    
    WKWebView *m_webView;
    WeakObjCPtr<id <_WKFindDelegate>> m_delegate;
    
    struct {
        bool webviewDidCountStringMatches : 1;
        bool webviewDidFindString : 1;
        bool webviewDidFailToFindString : 1;
    } m_delegateMethods;
};
    
} // namespace WebKit

#endif // WK_API_ENABLED

#endif // FindClient_h