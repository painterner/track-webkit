/*
 * Copyright (C) 2011, 2014 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MemoryPressureHandler.h"

#include "CSSValuePool.h"
#include "Document.h"
#include "FontCache.h"
#include "FontCascade.h"
#include "GCController.h"
#include "JSDOMWindow.h"
#include "MemoryCache.h"
#include "Page.h"
#include "PageCache.h"
#include "ScrollingThread.h"
#include "WorkerThread.h"
#include <wtf/CurrentTime.h>
#include <wtf/FastMalloc.h>
#include <wtf/Functional.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

WEBCORE_EXPORT bool MemoryPressureHandler::ReliefLogger::s_loggingEnabled = false;

MemoryPressureHandler& MemoryPressureHandler::singleton()
{
    static NeverDestroyed<MemoryPressureHandler> memoryPressureHandler;
    return memoryPressureHandler;
}

MemoryPressureHandler::MemoryPressureHandler() 
    : m_installed(false)
    , m_lastRespondTime(0)
    , m_lowMemoryHandler([this] (bool critical) { releaseMemory(critical); })
    , m_underMemoryPressure(false)
#if PLATFORM(IOS)
    // FIXME: Can we share more of this with OpenSource?
    , m_memoryPressureReason(MemoryPressureReasonNone)
    , m_clearPressureOnMemoryRelease(true)
    , m_releaseMemoryBlock(0)
    , m_observer(0)
#elif OS(LINUX)
    , m_eventFD(0)
    , m_pressureLevelFD(0)
    , m_threadID(0)
    , m_holdOffTimer(*this, &MemoryPressureHandler::holdOffTimerFired)
#endif
{
}

void MemoryPressureHandler::releaseNoncriticalMemory()
{
    {
        ReliefLogger log("Purge inactive FontData");
        FontCache::singleton().purgeInactiveFontData();
    }

    {
        ReliefLogger log("Clear WidthCaches");
        clearWidthCaches();
    }

    {
        ReliefLogger log("Discard Selector Query Cache");
        for (auto* document : Document::allDocuments())
            document->clearSelectorQueryCache();
    }

    {
        ReliefLogger log("Clearing JS string cache");
        JSDOMWindow::commonVM().stringCache.clear();
    }

    {
        ReliefLogger log("Evict MemoryCache dead resources");
        MemoryCache::singleton().pruneDeadResourcesToSize(0);
    }
}

void MemoryPressureHandler::releaseCriticalMemory()
{
    {
        ReliefLogger log("Empty the PageCache");
        // Right now, the only reason we call release critical memory while not under memory pressure is if the process is about to be suspended.
        PruningReason pruningReason = isUnderMemoryPressure() ? PruningReason::MemoryPressure : PruningReason::ProcessSuspended;
        PageCache::singleton().pruneToSizeNow(0, pruningReason);
    }

    {
        ReliefLogger log("Evict all MemoryCache resources");
        MemoryCache::singleton().evictResources();
    }

    {
        ReliefLogger log("Drain CSSValuePool");
        cssValuePool().drain();
    }

    {
        ReliefLogger log("Discard StyleResolvers");
        for (auto* document : Document::allDocuments())
            document->clearStyleResolver();
    }

    {
        ReliefLogger log("Discard all JIT-compiled code");
        gcController().discardAllCompiledCode();
    }
}

void MemoryPressureHandler::releaseMemory(bool critical)
{
    if (critical)
        releaseCriticalMemory();

    releaseNoncriticalMemory();

    platformReleaseMemory(critical);

    {
        ReliefLogger log("Release free FastMalloc memory");
        // FastMalloc has lock-free thread specific caches that can only be cleared from the thread itself.
        WorkerThread::releaseFastMallocFreeMemoryInAllThreads();
#if ENABLE(ASYNC_SCROLLING) && !PLATFORM(IOS)
        ScrollingThread::dispatch(WTF::releaseFastMallocFreeMemory);
#endif
        WTF::releaseFastMallocFreeMemory();
    }
}

#if !PLATFORM(COCOA) && !OS(LINUX)
void MemoryPressureHandler::install() { }
void MemoryPressureHandler::uninstall() { }
void MemoryPressureHandler::holdOff(unsigned) { }
void MemoryPressureHandler::respondToMemoryPressure(bool) { }
void MemoryPressureHandler::platformReleaseMemory(bool) { }
void MemoryPressureHandler::ReliefLogger::platformLog() { }
size_t MemoryPressureHandler::ReliefLogger::platformMemoryUsage() { return 0; }
#endif

} // namespace WebCore
