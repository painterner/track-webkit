/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2006 Apple Computer, Inc.

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef loader_h
#define loader_h

#include "SubresourceLoaderClient.h"
#include <wtf/HashMap.h>
#include "DeprecatedPtrList.h"

#ifdef __OBJC__
@class NSData;
#else
class NSData;
#endif

namespace WebCore {

    class CachedResource;
    class DocLoader;
    class Request;
    class String;

    class Loader : private SubresourceLoaderClient {
    public:
        Loader();
        ~Loader();

        void load(DocLoader*, CachedResource*, bool incremental = true);

        int numRequests(DocLoader*) const;
        void cancelRequests(DocLoader*);

        void removeBackgroundDecodingRequest(Request*);
        
    private:
        virtual void didReceiveResponse(SubresourceLoader*, const ResourceResponse&);
        virtual void didReceiveData(SubresourceLoader*, const char*, int);
        virtual void didFinishLoading(SubresourceLoader*);
        virtual void didFail(SubresourceLoader*, const ResourceError&);

        void servePendingRequests();

        DeprecatedPtrList<Request> m_requestsPending;
        typedef HashMap<RefPtr<SubresourceLoader>, Request*> RequestMap;
        RequestMap m_requestsLoading;

        DeprecatedPtrList<Request> m_requestsBackgroundDecoding;
    };

}

#endif
