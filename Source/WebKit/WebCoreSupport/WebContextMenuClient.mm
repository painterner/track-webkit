/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebContextMenuClient.h"

#import "WebElementDictionary.h"
#import "WebFrame.h"
#import "WebFrameInternal.h"
#import "WebHTMLView.h"
#import "WebHTMLViewInternal.h"
#import "WebNSPasteboardExtras.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import <WebCore/ContextMenu.h>
#import <WebCore/KURL.h>

using namespace WebCore;

@interface NSApplication (AppKitSecretsIKnowAbout)
- (void)speakString:(NSString *)string;
@end

WebContextMenuClient::WebContextMenuClient(WebView *webView) 
    : m_webView(webView)
{
}

void WebContextMenuClient::contextMenuDestroyed()
{
    delete this;
}

NSMutableArray* WebContextMenuClient::getCustomMenuFromDefaultItems(ContextMenu* defaultMenu)
{
    id delegate = [m_webView UIDelegate];
    if ([delegate respondsToSelector:@selector(webView:contextMenuItemsForElement:defaultMenuItems:)]) {
        NSDictionary *element = [[[WebElementDictionary alloc] initWithHitTestResult:defaultMenu->hitTestResult()] autorelease];
        NSMutableArray *defaultMenuItems = defaultMenu->platformDescription();
        NSMutableArray *newMenuItems = [[delegate webView:m_webView contextMenuItemsForElement:element defaultMenuItems:defaultMenuItems] mutableCopy];
        
        // Versions of Mail compiled with older WebKits will end up without three context menu items, 
        // though with the separators between them. Here we check for that problem and reinsert the 
        // three missing items. This shouldn't affect any clients other than Mail since the tags for 
        // the three items were not public API. We can remove this code when we no longer support 
        // previously-built versions of Mail on Tiger. See 4498606 for more details.
        if ([newMenuItems count] &&
            ([[defaultMenuItems objectAtIndex:0] tag] == WebMenuItemTagSearchInSpotlight) &&
            ([[newMenuItems objectAtIndex:0] isSeparatorItem])) {
            ASSERT([[newMenuItems objectAtIndex:1] isSeparatorItem]);
            ASSERT([[defaultMenuItems objectAtIndex:1] tag] == WebMenuItemTagSearchWeb);
            ASSERT([[defaultMenuItems objectAtIndex:2] isSeparatorItem]);
            ASSERT([[defaultMenuItems objectAtIndex:3] tag] == WebMenuItemTagLookUpInDictionary);
            ASSERT([[defaultMenuItems objectAtIndex:4] isSeparatorItem]);
            [newMenuItems insertObject:[defaultMenuItems objectAtIndex:0] atIndex:0];
            [newMenuItems insertObject:[defaultMenuItems objectAtIndex:1] atIndex:1];
            [newMenuItems insertObject:[defaultMenuItems objectAtIndex:3] atIndex:3];
        }
        
        return [newMenuItems autorelease];
    }
    return nil;
}

void WebContextMenuClient::contextMenuItemSelected(ContextMenuItem* item, const ContextMenu* parentMenu)
{
    id delegate = [m_webView UIDelegate];
    if ([delegate respondsToSelector:@selector(webView:contextMenuItemSelected:forElement:)]) {
        NSDictionary *element = [[WebElementDictionary alloc] initWithHitTestResult:parentMenu->hitTestResult()];
        [delegate webView:m_webView contextMenuItemSelected:item->releasePlatformDescription() forElement:element];
        [element release];
    }
}

void WebContextMenuClient::downloadURL(const KURL& url)
{
    [m_webView _downloadURL:url.getNSURL()];
}

void WebContextMenuClient::copyImageToClipboard(const HitTestResult& hitTestResult)
{
    NSDictionary *element = [[[WebElementDictionary alloc] initWithHitTestResult:hitTestResult] autorelease];
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *types = [NSPasteboard _web_writableTypesForImageIncludingArchive:(hitTestResult.innerNonSharedNode() != 0)];
    [pasteboard declareTypes:types owner:m_webView];
    [m_webView _writeImageForElement:element withPasteboardTypes:types toPasteboard:pasteboard];
}

void WebContextMenuClient::searchWithSpotlight()
{
    [m_webView _searchWithSpotlightFromMenu:nil];
}

void WebContextMenuClient::lookUpInDictionary(Frame* frame)
{
    WebHTMLView* htmlView = (WebHTMLView*)[[kit(frame) frameView] documentView];
    if(![htmlView isKindOfClass:[WebHTMLView class]])
        return;
    [htmlView _lookUpInDictionaryFromMenu:nil];
}

void WebContextMenuClient::speak(const String& string)
{
    [NSApp speakString:[[(NSString*)string copy] autorelease]];
}

void WebContextMenuClient::stopSpeaking()
{
    [NSApp stopSpeaking];
}
