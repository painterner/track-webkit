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
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
 
#ifndef HTMLTextFieldInnerElement_H
#define HTMLTextFieldInnerElement_H

#include "HTMLDivElement.h"

namespace WebCore {

class String;

class HTMLTextFieldInnerElement : public HTMLDivElement
{
public:
    HTMLTextFieldInnerElement(Document*, Node* shadowParent = 0);
    
    virtual bool isMouseFocusable() const { return false; } 
    virtual bool isShadowNode() const { return m_shadowParent; }
    virtual Node* shadowParentNode() { return m_shadowParent; }
    void setShadowParentNode(Node* node) { m_shadowParent = node; }
    
private:
    Node* m_shadowParent;
};

class HTMLTextFieldInnerTextElement : public HTMLTextFieldInnerElement {
public:
    HTMLTextFieldInnerTextElement(Document*, Node* shadowParent);        
    virtual void defaultEventHandler(Event*);
};

class HTMLSearchFieldResultsButtonElement : public HTMLTextFieldInnerElement {
public:
    HTMLSearchFieldResultsButtonElement(Document*);
    virtual void defaultEventHandler(Event*);
};

class HTMLSearchFieldCancelButtonElement : public HTMLTextFieldInnerElement {
public:
    HTMLSearchFieldCancelButtonElement(Document*);
    virtual void defaultEventHandler(Event*);
};

} //namespace

#endif
