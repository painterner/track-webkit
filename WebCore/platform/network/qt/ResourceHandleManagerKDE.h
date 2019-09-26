/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * All rights reserved.
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

#ifndef ResourceHandleManager_H
#define ResourceHandleManager_H

#include <QMap>
#include <QObject>

#include "ResourceHandle.h"

namespace KIO {
class Job;
}
class KJob;

namespace WebCore {

class FrameQtClient;

class ResourceHandleManager : public QObject
{
    Q_OBJECT
public:
    static ResourceHandleManager* self();

    void add(ResourceHandle*, FrameQtClient*);
    void cancel(ResourceHandle*);

public Q_SLOTS:
    void slotData(KIO::Job*, const QByteArray& data);
    void slotMimetype(KIO::Job*, const QString& type);
    void slotResult(KJob*);
    void deliverJobData(QtJob* , const QByteArray&);

private:
    ResourceHandleManager();
    ~ResourceHandleManager();

    void remove(ResourceHandle*);

    // KIO Job <-> WebKit Job mapping
    QMap<ResourceHandle*, KIO::Job*> m_jobToKioMap;
    QMap<KIO::Job*, ResourceHandle*> m_kioToJobMap;

    FrameQtClient* m_frameClient;
};

}

#endif