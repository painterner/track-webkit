// -*- c-basic-offset: 4 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006 Jon Shier (jshier@iastate.edu)
 *  Copyright (C) 2003, 2004, 2005, 2006 Apple Computer, Inc.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "kjs_window.h"

#include "Base64.h"
#include "Chrome.h"
#include "CString.h"
#include "DOMWindow.h"
#include "Element.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLDocument.h"
#include "JSCSSRule.h"
#include "JSCSSValue.h"
#include "JSDOMWindow.h"
#include "JSEvent.h"
#include "JSHTMLOptionElementConstructor.h"
#include "JSMutationEvent.h"
#include "JSNode.h"
#include "JSNodeFilter.h"
#include "JSRange.h"
#include "JSXMLHttpRequest.h"
#include "Logging.h"
#include "Page.h"
#include "PlugInInfoStore.h"
#include "RenderView.h"
#include "Screen.h"
#include "SelectionController.h"
#include "Settings.h"
#include "WindowFeatures.h"
#include "htmlediting.h"
#include "kjs_css.h"
#include "kjs_events.h"
#include "kjs_navigator.h"
#include "kjs_proxy.h"
#include "kjs_traversal.h"
#include <wtf/MathExtras.h>

#if XSLT_SUPPORT
#include "JSXSLTProcessor.h"
#endif

using namespace WebCore;
using namespace EventNames;

namespace KJS {

static int lastUsedTimeoutId;

static int timerNestingLevel = 0;
const int cMaxTimerNestingLevel = 5;
const double cMinimumTimerInterval = 0.010;

class DOMWindowTimer : public TimerBase {
public:
    DOMWindowTimer(int timeoutId, int nestingLevel, Window* o, ScheduledAction* a)
        : m_timeoutId(timeoutId), m_nestingLevel(nestingLevel), m_object(o), m_action(a) { }
    virtual ~DOMWindowTimer() { delete m_action; }

    int timeoutId() const { return m_timeoutId; }
    
    int nestingLevel() const { return m_nestingLevel; }
    void setNestingLevel(int n) { m_nestingLevel = n; }
    
    ScheduledAction* action() const { return m_action; }
    ScheduledAction* takeAction() { ScheduledAction* a = m_action; m_action = 0; return a; }

private:
    virtual void fired();

    int m_timeoutId;
    int m_nestingLevel;
    Window* m_object;
    ScheduledAction* m_action;
};

class PausedTimeout {
public:
    int timeoutId;
    int nestingLevel;
    double nextFireInterval;
    double repeatInterval;
    ScheduledAction *action;
};

////////////////////// History Object ////////////////////////

  class History : public DOMObject {
    friend class HistoryFunc;
  public:
    History(ExecState *exec, Frame *f)
      : m_frame(f)
    {
      setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
    }
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Back, Forward, Go, Length };
    virtual UString toString(ExecState *exec) const;
    void disconnectFrame() { m_frame = 0; }
  private:
    Frame* m_frame;
  };


  class FrameArray : public DOMObject {
  public:
    FrameArray(ExecState *exec, Frame *f)
      : m_frame(f)
    {
      setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
    }
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token);
    virtual UString toString(ExecState *exec) const;
    enum { Length, Location };
    void disconnectFrame() { m_frame = 0; }
  private:
    static JSValue *indexGetter(ExecState *, JSObject *, const Identifier&, const PropertySlot&);
    static JSValue *nameGetter(ExecState *, JSObject *, const Identifier&, const PropertySlot&);

    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    Frame* m_frame;
  };

}

#include "kjs_window.lut.h"

namespace KJS {

////////////////////// Screen Object ////////////////////////

// table for screen object
/*
@begin ScreenTable 7
  height        Screen::Height          DontEnum|ReadOnly
  width         Screen::Width           DontEnum|ReadOnly
  colorDepth    Screen::ColorDepth      DontEnum|ReadOnly
  pixelDepth    Screen::PixelDepth      DontEnum|ReadOnly
  availLeft     Screen::AvailLeft       DontEnum|ReadOnly
  availTop      Screen::AvailTop        DontEnum|ReadOnly
  availHeight   Screen::AvailHeight     DontEnum|ReadOnly
  availWidth    Screen::AvailWidth      DontEnum|ReadOnly
@end
*/

const ClassInfo Screen::info = { "Screen", 0, &ScreenTable, 0 };

// We set the object prototype so that toString is implemented
Screen::Screen(ExecState* exec, Frame* f)
    : m_frame(f)
{
    setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

bool Screen::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticValueSlot<Screen, JSObject>(exec, &ScreenTable, this, propertyName, slot);
}

JSValue* Screen::getValueProperty(ExecState*, int token) const
{
  Widget* widget = m_frame ? m_frame->view() : 0;  

  switch (token) {
  case Height:
    return jsNumber(screenRect(widget).height());
  case Width:
    return jsNumber(screenRect(widget).width());
  case ColorDepth:
  case PixelDepth:
    return jsNumber(screenDepth(widget));
  case AvailLeft:
    return jsNumber(screenAvailableRect(widget).x());
  case AvailTop:
    return jsNumber(screenAvailableRect(widget).y());
  case AvailHeight:
    return jsNumber(screenAvailableRect(widget).height());
  case AvailWidth:
    return jsNumber(screenAvailableRect(widget).width());
  default:
    return jsUndefined();
  }
}

////////////////////// Window Object ////////////////////////

const ClassInfo Window::info = { "Window", 0, &WindowTable, 0 };

/*
@begin WindowTable 103
  atob          Window::AToB            DontDelete|Function 1
  btoa          Window::BToA            DontDelete|Function 1
  closed        Window::Closed          DontDelete|ReadOnly
  crypto        Window::Crypto          DontDelete|ReadOnly
  defaultStatus Window::DefaultStatus   DontDelete
  defaultstatus Window::DefaultStatus   DontDelete
  status        Window::Status          DontDelete
  DOMException  Window::DOMException    DontDelete
  frames        Window::Frames          DontDelete|ReadOnly
  history       Window::History_        DontDelete|ReadOnly
  event         Window::Event_          DontDelete
  innerHeight   Window::InnerHeight     DontDelete|ReadOnly
  innerWidth    Window::InnerWidth      DontDelete|ReadOnly
  length        Window::Length          DontDelete|ReadOnly
  location      Window::Location_       DontDelete
  locationbar   Window::Locationbar     DontDelete
  name          Window::Name            DontDelete
  navigator     Window::Navigator_      DontDelete|ReadOnly
  clientInformation     Window::ClientInformation       DontDelete|ReadOnly
  menubar       Window::Menubar         DontDelete|ReadOnly
  offscreenBuffering    Window::OffscreenBuffering      DontDelete|ReadOnly
  opener        Window::Opener          DontDelete|ReadOnly
  outerHeight   Window::OuterHeight     DontDelete|ReadOnly
  outerWidth    Window::OuterWidth      DontDelete|ReadOnly
  pageXOffset   Window::PageXOffset     DontDelete|ReadOnly
  pageYOffset   Window::PageYOffset     DontDelete|ReadOnly
  parent        Window::Parent          DontDelete|ReadOnly
  personalbar   Window::Personalbar     DontDelete|ReadOnly
  screenX       Window::ScreenX         DontDelete|ReadOnly
  screenY       Window::ScreenY         DontDelete|ReadOnly
  screenLeft    Window::ScreenLeft      DontDelete|ReadOnly
  screenTop     Window::ScreenTop       DontDelete|ReadOnly
  scrollbars    Window::Scrollbars      DontDelete|ReadOnly
  statusbar     Window::Statusbar       DontDelete|ReadOnly
  toolbar       Window::Toolbar         DontDelete|ReadOnly
  scroll        Window::Scroll          DontDelete|Function 2
  scrollBy      Window::ScrollBy        DontDelete|Function 2
  scrollTo      Window::ScrollTo        DontDelete|Function 2
  scrollX       Window::ScrollX         DontDelete|ReadOnly
  scrollY       Window::ScrollY         DontDelete|ReadOnly
  moveBy        Window::MoveBy          DontDelete|Function 2
  moveTo        Window::MoveTo          DontDelete|Function 2
  resizeBy      Window::ResizeBy        DontDelete|Function 2
  resizeTo      Window::ResizeTo        DontDelete|Function 2
  self          Window::Self            DontDelete|ReadOnly
  window        Window::Window_         DontDelete|ReadOnly
  top           Window::Top             DontDelete|ReadOnly
  screen        Window::Screen_         DontDelete|ReadOnly
  Image         Window::Image           DontDelete
  Option        Window::Option          DontDelete
  XMLHttpRequest        Window::XMLHttpRequest  DontDelete
  XSLTProcessor Window::XSLTProcessor_  DontDelete
  alert         Window::Alert           DontDelete|Function 1
  confirm       Window::Confirm         DontDelete|Function 1
  prompt        Window::Prompt          DontDelete|Function 2
  open          Window::Open            DontDelete|Function 3
  print         Window::Print           DontDelete|Function 2
  setTimeout    Window::SetTimeout      DontDelete|Function 2
  clearTimeout  Window::ClearTimeout    DontDelete|Function 1
  focus         Window::Focus           DontDelete|Function 0
  getSelection  Window::GetSelection    DontDelete|Function 0
  blur          Window::Blur            DontDelete|Function 0
  close         Window::Close           DontDelete|Function 0
  setInterval   Window::SetInterval     DontDelete|Function 2
  clearInterval Window::ClearInterval   DontDelete|Function 1
  captureEvents Window::CaptureEvents   DontDelete|Function 0
  releaseEvents Window::ReleaseEvents   DontDelete|Function 0
# Warning, when adding a function to this object you need to add a case in Window::get
  addEventListener      Window::AddEventListener        DontDelete|Function 3
  removeEventListener   Window::RemoveEventListener     DontDelete|Function 3
  onabort       Window::Onabort         DontDelete
  onblur        Window::Onblur          DontDelete
  onchange      Window::Onchange        DontDelete
  onclick       Window::Onclick         DontDelete
  ondblclick    Window::Ondblclick      DontDelete
  ondragdrop    Window::Ondragdrop      DontDelete
  onerror       Window::Onerror         DontDelete
  onfocus       Window::Onfocus         DontDelete
  onkeydown     Window::Onkeydown       DontDelete
  onkeypress    Window::Onkeypress      DontDelete
  onkeyup       Window::Onkeyup         DontDelete
  onload        Window::Onload          DontDelete
  onmousedown   Window::Onmousedown     DontDelete
  onmousemove   Window::Onmousemove     DontDelete
  onmouseout    Window::Onmouseout      DontDelete
  onmouseover   Window::Onmouseover     DontDelete
  onmouseup     Window::Onmouseup       DontDelete
  onmousewheel  Window::OnWindowMouseWheel      DontDelete
  onmove        Window::Onmove          DontDelete
  onreset       Window::Onreset         DontDelete
  onresize      Window::Onresize        DontDelete
  onscroll      Window::Onscroll        DontDelete
  onsearch      Window::Onsearch        DontDelete
  onselect      Window::Onselect        DontDelete
  onsubmit      Window::Onsubmit        DontDelete
  onunload      Window::Onunload        DontDelete
  onbeforeunload Window::Onbeforeunload DontDelete
  frameElement  Window::FrameElement    DontDelete|ReadOnly
  showModalDialog Window::ShowModalDialog    DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(WindowFunc)

Window::Window(DOMWindow* window)
  : m_frame(window->frame())
  , screen(0)
  , history(0)
  , frames(0)
  , loc(0)
  , m_selection(0)
  , m_locationbar(0)
  , m_menubar(0)
  , m_personalbar(0)
  , m_scrollbars(0)
  , m_statusbar(0)
  , m_toolbar(0)
  , m_evt(0)
  , m_returnValueSlot(0)
{
}

Window::~Window()
{
    clearAllTimeouts();

    // Clear any backpointers to the window

    ListenersMap::iterator i2 = jsEventListeners.begin();
    ListenersMap::iterator e2 = jsEventListeners.end();
    for (; i2 != e2; ++i2)
        i2->second->clearWindowObj();
    i2 = jsHTMLEventListeners.begin();
    e2 = jsHTMLEventListeners.end();
    for (; i2 != e2; ++i2)
        i2->second->clearWindowObj();

    UnprotectedListenersMap::iterator i1 = jsUnprotectedEventListeners.begin();
    UnprotectedListenersMap::iterator e1 = jsUnprotectedEventListeners.end();
    for (; i1 != e1; ++i1)
        i1->second->clearWindowObj();
    i1 = jsUnprotectedHTMLEventListeners.begin();
    e1 = jsUnprotectedHTMLEventListeners.end();
    for (; i1 != e1; ++i1)
        i1->second->clearWindowObj();
}

DOMWindow* Window::impl() const
{
     return m_frame->domWindow();
}

ScriptInterpreter *Window::interpreter() const
{
    return m_frame->scriptProxy()->interpreter();
}

Window *Window::retrieveWindow(Frame *f)
{
    JSObject *o = retrieve(f)->getObject();

    ASSERT(o || !f->settings()->isJavaScriptEnabled());
    return static_cast<Window *>(o);
}

Window *Window::retrieveActive(ExecState *exec)
{
    JSValue *imp = exec->dynamicInterpreter()->globalObject();
    ASSERT(imp);
    return static_cast<Window*>(imp);
}

JSValue *Window::retrieve(Frame *p)
{
    ASSERT(p);
    if (KJSProxy *proxy = p->scriptProxy())
        return proxy->interpreter()->globalObject(); // the Global object is the "window"
  
    return jsUndefined(); // This can happen with JS disabled on the domain of that window
}

Location *Window::location() const
{
  if (!loc)
    loc = new Location(m_frame);
  return loc;
}

Selection *Window::selection() const
{
  if (!m_selection)
    m_selection = new Selection(m_frame);
  return m_selection;
}

BarInfo *Window::locationbar(ExecState *exec) const
{
  if (!m_locationbar)
    m_locationbar = new BarInfo(exec, m_frame, BarInfo::Locationbar);
  return m_locationbar;
}

BarInfo *Window::menubar(ExecState *exec) const
{
  if (!m_menubar)
    m_menubar = new BarInfo(exec, m_frame, BarInfo::Menubar);
  return m_menubar;
}

BarInfo *Window::personalbar(ExecState *exec) const
{
  if (!m_personalbar)
    m_personalbar = new BarInfo(exec, m_frame, BarInfo::Personalbar);
  return m_personalbar;
}

BarInfo *Window::statusbar(ExecState *exec) const
{
  if (!m_statusbar)
    m_statusbar = new BarInfo(exec, m_frame, BarInfo::Statusbar);
  return m_statusbar;
}

BarInfo *Window::toolbar(ExecState *exec) const
{
  if (!m_toolbar)
    m_toolbar = new BarInfo(exec, m_frame, BarInfo::Toolbar);
  return m_toolbar;
}

BarInfo *Window::scrollbars(ExecState *exec) const
{
  if (!m_scrollbars)
    m_scrollbars = new BarInfo(exec, m_frame, BarInfo::Scrollbars);
  return m_scrollbars;
}

// reference our special objects during garbage collection
void Window::mark()
{
  JSObject::mark();
  if (screen && !screen->marked())
    screen->mark();
  if (history && !history->marked())
    history->mark();
  if (frames && !frames->marked())
    frames->mark();
  if (loc && !loc->marked())
    loc->mark();
  if (m_selection && !m_selection->marked())
    m_selection->mark();
  if (m_locationbar && !m_locationbar->marked())
    m_locationbar->mark();
  if (m_menubar && !m_menubar->marked())
    m_menubar->mark();
  if (m_personalbar && !m_personalbar->marked())
    m_personalbar->mark();
  if (m_scrollbars && !m_scrollbars->marked())
    m_scrollbars->mark();
  if (m_statusbar && !m_statusbar->marked())
    m_statusbar->mark();
  if (m_toolbar && !m_toolbar->marked())
    m_toolbar->mark();
}

UString Window::toString(ExecState *) const
{
  return "[object Window]";
}

static bool allowPopUp(ExecState *exec, Window *window)
{
    return window->frame()
        && (window->frame()->settings()->JavaScriptCanOpenWindowsAutomatically()
            || static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture());
}

static HashMap<String, String> parseModalDialogFeatures(ExecState *exec, JSValue *featuresArg)
{
    HashMap<String, String> map;

    Vector<String> features = String(featuresArg->toString(exec)).split(';');
    Vector<String>::const_iterator end = features.end();
    for (Vector<String>::const_iterator it = features.begin(); it != end; ++it) {
        String s = *it;
        int pos = s.find('=');
        int colonPos = s.find(':');
        if (pos >= 0 && colonPos >= 0)
            continue; // ignore any strings that have both = and :
        if (pos < 0)
            pos = colonPos;
        if (pos < 0) {
            // null string for value means key without value
            map.set(s.stripWhiteSpace().lower(), String());
        } else {
            String key = s.left(pos).stripWhiteSpace().lower();
            String val = s.substring(pos + 1).stripWhiteSpace().lower();
            int spacePos = val.find(' ');
            if (spacePos != -1)
                val = val.left(spacePos);
            map.set(key, val);
        }
    }

    return map;
}

static bool boolFeature(const HashMap<String, String>& features, const char* key, bool defaultValue = false)
{
    HashMap<String, String>::const_iterator it = features.find(key);
    if (it == features.end())
        return defaultValue;
    const String& value = it->second;
    return value.isNull() || value == "1" || value == "yes" || value == "on";
}

static float floatFeature(const HashMap<String, String> &features, const char *key, float min, float max, float defaultValue)
{
    HashMap<String, String>::const_iterator it = features.find(key);
    if (it == features.end())
        return defaultValue;
    // FIXME: Can't distinguish "0q" from string with no digits in it -- both return d == 0 and ok == false.
    // Would be good to tell them apart somehow since string with no digits should be default value and
    // "0q" should be minimum value.
    bool ok;
    double d = it->second.toDouble(&ok);
    if ((d == 0 && !ok) || isnan(d))
        return defaultValue;
    if (d < min || max <= min)
        return min;
    if (d > max)
        return max;
    return static_cast<int>(d);
}

static Frame* createNewWindow(ExecState* exec, Window* openerWindow, const DeprecatedString &URL,
    const String& frameName, const WindowFeatures& windowFeatures, JSValue* dialogArgs)
{
    Frame* openerFrame = openerWindow->frame();
    Frame* activeFrame = Window::retrieveActive(exec)->frame();

    ResourceRequest request = ResourceRequest(KURL(""));
    if (activeFrame)
        request.setHTTPReferrer(activeFrame->loader()->outgoingReferrer());
    FrameLoadRequest frameRequest(request, frameName);

    // FIXME: It's much better for client API if a new window starts with a URL, here where we
    // know what URL we are going to open. Unfortunately, this code passes the empty string
    // for the URL, but there's a reason for that. Before loading we have to set up the opener,
    // openedByJS, and dialogArguments values. Also, to decide whether to use the URL we currently
    // do an isSafeScript call using the window we create, which can't be done before creating it.
    // We'd have to resolve all those issues to pass the URL instead of "".

    Frame* newFrame = openerFrame->loader()->createWindow(frameRequest, windowFeatures);
    if (!newFrame)
        return 0;

    Window* newWindow = Window::retrieveWindow(newFrame);

    newFrame->loader()->setOpener(openerFrame);
    newFrame->loader()->setOpenedByJavaScript();
    if (dialogArgs)
        newWindow->putDirect("dialogArguments", dialogArgs);

    Document *activeDoc = activeFrame ? activeFrame->document() : 0;
    if (!URL.isEmpty() && activeDoc) {
        DeprecatedString completedURL = activeDoc->completeURL(URL);
        if (!completedURL.startsWith("javascript:", false) || newWindow->isSafeScript(exec)) {
            bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
            newFrame->loader()->changeLocation(completedURL, activeFrame->loader()->outgoingReferrer(), false, userGesture);
        }
    }

    return newFrame;
}

static bool canShowModalDialog(const Window *window)
{
    if (Frame* frame = window->frame())
        return frame->page()->chrome()->canRunModal();
    return false;
}

static bool canShowModalDialogNow(const Window *window)
{
    if (Frame* frame = window->frame())
        return frame->page()->chrome()->canRunModalNow();
    return false;
}

static JSValue* showModalDialog(ExecState* exec, Window* openerWindow, const List& args)
{
    UString URL = args[0]->toString(exec);

    if (!canShowModalDialogNow(openerWindow) || !allowPopUp(exec, openerWindow))
        return jsUndefined();
    
    const HashMap<String, String> features = parseModalDialogFeatures(exec, args[2]);

    bool trusted = false;

    WindowFeatures wargs;

    // The following features from Microsoft's documentation are not implemented:
    // - default font settings
    // - width, height, left, and top specified in units other than "px"
    // - edge (sunken or raised, default is raised)
    // - dialogHide: trusted && boolFeature(features, "dialoghide"), makes dialog hide when you print
    // - help: boolFeature(features, "help", true), makes help icon appear in dialog (what does it do on Windows?)
    // - unadorned: trusted && boolFeature(features, "unadorned");

    FloatRect screenRect = screenAvailableRect(openerWindow->frame()->view());

    wargs.width = floatFeature(features, "dialogwidth", 100, screenRect.width(), 620); // default here came from frame size of dialog in MacIE
    wargs.widthSet = true;
    wargs.height = floatFeature(features, "dialogheight", 100, screenRect.height(), 450); // default here came from frame size of dialog in MacIE
    wargs.heightSet = true;

    wargs.x = floatFeature(features, "dialogleft", screenRect.x(), screenRect.right() - wargs.width, -1);
    wargs.xSet = wargs.x > 0;
    wargs.y = floatFeature(features, "dialogtop", screenRect.y(), screenRect.bottom() - wargs.height, -1);
    wargs.ySet = wargs.y > 0;

    if (boolFeature(features, "center", true)) {
        if (!wargs.xSet) {
            wargs.x = screenRect.x() + (screenRect.width() - wargs.width) / 2;
            wargs.xSet = true;
        }
        if (!wargs.ySet) {
            wargs.y = screenRect.y() + (screenRect.height() - wargs.height) / 2;
            wargs.ySet = true;
        }
    }

    wargs.dialog = true;
    wargs.resizable = boolFeature(features, "resizable");
    wargs.scrollbarsVisible = boolFeature(features, "scroll", true);
    wargs.statusBarVisible = boolFeature(features, "status", !trusted);
    wargs.menuBarVisible = false;
    wargs.toolBarVisible = false;
    wargs.locationBarVisible = false;
    wargs.fullscreen = false;
    
    Frame* dialogFrame = createNewWindow(exec, openerWindow, URL, "", wargs, args[1]);
    if (!dialogFrame)
        return jsUndefined();

    Window* dialogWindow = Window::retrieveWindow(dialogFrame);

    // Get the return value either just before clearing the dialog window's
    // properties (in Window::clear), or when on return from runModal.
    JSValue* returnValue = 0;
    dialogWindow->setReturnValueSlot(&returnValue);
    dialogFrame->page()->chrome()->runModal();
    dialogWindow->setReturnValueSlot(0);

    // If we don't have a return value, get it now.
    // Either Window::clear was not called yet, or there was no return value,
    // and in that case, there's no harm in trying again (no benefit either).
    if (!returnValue)
        returnValue = dialogWindow->getDirect("returnValue");

    return returnValue ? returnValue : jsUndefined();
}

JSValue *Window::getValueProperty(ExecState *exec, int token) const
{
   ASSERT(token == Closed || m_frame);

   switch (token) {
   case Closed:
      return jsBoolean(!m_frame);
   case Crypto:
      return jsUndefined(); // ###
   case DefaultStatus:
      return jsString(UString(m_frame->jsDefaultStatusBarText()));
   case DOMException:
      return getDOMExceptionConstructor(exec);
   case Status:
      return jsString(UString(m_frame->jsStatusBarText()));
    case Frames:
      if (!frames)
        frames = new FrameArray(exec, m_frame);
      return frames;
    case History_:
      if (!history)
        history = new History(exec, m_frame);
      return history;
    case Event_:
      if (!m_evt)
        return jsUndefined();
      return toJS(exec, m_evt);
    case InnerHeight:
      if (!m_frame->view())
        return jsUndefined();
      return jsNumber(m_frame->view()->visibleHeight());
    case InnerWidth:
      if (!m_frame->view())
        return jsUndefined();
      return jsNumber(m_frame->view()->visibleWidth());
    case Length:
      return jsNumber(m_frame->tree()->childCount());
    case Location_:
      return location();
    case Name:
      return jsString(m_frame->tree()->name());
    case Navigator_:
    case ClientInformation: {
      // Store the navigator in the object so we get the same one each time.
      Navigator *n = new Navigator(exec, m_frame);
      // FIXME: this will make the "navigator" object accessible from windows that fail
      // the security check the first time, but not subsequent times, seems weird.
      const_cast<Window *>(this)->putDirect("navigator", n, DontDelete|ReadOnly);
      const_cast<Window *>(this)->putDirect("clientInformation", n, DontDelete|ReadOnly);
      return n;
    }
    case Locationbar:
      return locationbar(exec);
    case Menubar:
      return menubar(exec);
    case OffscreenBuffering:
      return jsBoolean(true);
    case Opener:
      if (m_frame->loader()->opener())
        return retrieve(m_frame->loader()->opener());
      return jsNull();
    case OuterHeight:
        return jsNumber(m_frame->page()->chrome()->windowRect().height());
    case OuterWidth:
        return jsNumber(m_frame->page()->chrome()->windowRect().width());
    case PageXOffset:
      if (!m_frame->view())
        return jsUndefined();
      updateLayout();
      return jsNumber(m_frame->view()->contentsX());
    case PageYOffset:
      if (!m_frame->view())
        return jsUndefined();
      updateLayout();
      return jsNumber(m_frame->view()->contentsY());
    case Parent:
      return retrieve(m_frame->tree()->parent() ? m_frame->tree()->parent() : m_frame);
    case Personalbar:
      return personalbar(exec);
    case ScreenLeft:
    case ScreenX:
      return jsNumber(m_frame->page()->chrome()->windowRect().x());
    case ScreenTop:
    case ScreenY:
      return jsNumber(m_frame->page()->chrome()->windowRect().y());
    case ScrollX:
      if (!m_frame->view())
        return jsUndefined();
      updateLayout();
      return jsNumber(m_frame->view()->contentsX());
    case ScrollY:
      if (!m_frame->view())
        return jsUndefined();
      updateLayout();
      return jsNumber(m_frame->view()->contentsY());
    case Scrollbars:
      return scrollbars(exec);
    case Statusbar:
      return statusbar(exec);
    case Toolbar:
      return toolbar(exec);
    case Self:
    case Window_:
      return retrieve(m_frame);
    case Top:
      return retrieve(m_frame->page()->mainFrame());
    case Screen_:
      if (!screen)
        screen = new Screen(exec, m_frame);
      return screen;
    case Image:
      // FIXME: this property (and the few below) probably shouldn't create a new object every
      // time
      return new ImageConstructorImp(exec, m_frame->document());
    case Option:
      return new JSHTMLOptionElementConstructor(exec, m_frame->document());
    case XMLHttpRequest:
      return new JSXMLHttpRequestConstructorImp(exec, m_frame->document());
#ifdef XSLT_SUPPORT
    case XSLTProcessor_:
      return new XSLTProcessorConstructorImp(exec);
#else
    case XSLTProcessor_:
      return jsUndefined();
#endif
    case FrameElement:
      if (Document* doc = m_frame->document())
        if (Element* fe = doc->ownerElement())
          if (checkNodeSecurity(exec, fe))
            return toJS(exec, fe);
      return jsUndefined();
   }

   if (!isSafeScript(exec))
     return jsUndefined();

   switch (token) {
   case Onabort:
     return getListener(exec, abortEvent);
   case Onblur:
     return getListener(exec, blurEvent);
   case Onchange:
     return getListener(exec, changeEvent);
   case Onclick:
     return getListener(exec, clickEvent);
   case Ondblclick:
     return getListener(exec, dblclickEvent);
   case Ondragdrop:
     return getListener(exec, khtmlDragdropEvent);
   case Onerror:
     return getListener(exec, errorEvent);
   case Onfocus:
     return getListener(exec, focusEvent);
   case Onkeydown:
     return getListener(exec, keydownEvent);
   case Onkeypress:
     return getListener(exec, keypressEvent);
   case Onkeyup:
     return getListener(exec, keyupEvent);
   case Onload:
     return getListener(exec, loadEvent);
   case Onmousedown:
     return getListener(exec, mousedownEvent);
   case Onmousemove:
     return getListener(exec, mousemoveEvent);
   case Onmouseout:
     return getListener(exec, mouseoutEvent);
   case Onmouseover:
     return getListener(exec, mouseoverEvent);
   case Onmouseup:
     return getListener(exec, mouseupEvent);
   case OnWindowMouseWheel:
     return getListener(exec, mousewheelEvent);
   case Onmove:
     return getListener(exec, khtmlMoveEvent);
   case Onreset:
     return getListener(exec, resetEvent);
   case Onresize:
     return getListener(exec,resizeEvent);
   case Onscroll:
     return getListener(exec,scrollEvent);
   case Onsearch:
     return getListener(exec,searchEvent);
   case Onselect:
     return getListener(exec,selectEvent);
   case Onsubmit:
     return getListener(exec,submitEvent);
   case Onbeforeunload:
      return getListener(exec, beforeunloadEvent);
    case Onunload:
     return getListener(exec, unloadEvent);
   }
   ASSERT(0);
   return jsUndefined();
}

JSValue* Window::childFrameGetter(ExecState*, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
    return retrieve(static_cast<Window*>(slot.slotBase())->m_frame->tree()->child(AtomicString(propertyName)));
}

JSValue* Window::indexGetter(ExecState*, JSObject*, const Identifier&, const PropertySlot& slot)
{
    return retrieve(static_cast<Window*>(slot.slotBase())->m_frame->tree()->child(slot.index()));
}

JSValue *Window::namedItemGetter(ExecState *exec, JSObject *originalObject, const Identifier& propertyName, const PropertySlot& slot)
{
  Window *thisObj = static_cast<Window *>(slot.slotBase());
  Document *doc = thisObj->m_frame->document();
  ASSERT(thisObj->isSafeScript(exec) && doc && doc->isHTMLDocument());

  String name = propertyName;
  RefPtr<WebCore::HTMLCollection> collection = doc->windowNamedItems(name);
  if (collection->length() == 1)
    return toJS(exec, collection->firstItem());
  else 
    return getHTMLCollection(exec, collection.get());
}

bool Window::getOverridePropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
  // we don't want any properties other than "closed" on a closed window
  if (!m_frame) {
    if (propertyName == "closed") {
      slot.setStaticEntry(this, Lookup::findEntry(&WindowTable, "closed"), staticValueGetter<Window>);
      return true;
    }
    if (propertyName == "close") {
      const HashEntry* entry = Lookup::findEntry(&WindowTable, propertyName);
      slot.setStaticEntry(this, entry, staticFunctionGetter<WindowFunc>);
      return true;
    }

    slot.setUndefined(this);
    return true;
  }

  // Look for overrides first
  JSValue **val = getDirectLocation(propertyName);
  if (val) {
    if (isSafeScript(exec))
      slot.setValueSlot(this, val);
    else
      slot.setUndefined(this);
    return true;
  }
  
  return false;
}

bool Window::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  // Check for child frames by name before built-in properties to
  // match Mozilla. This does not match IE, but some sites end up
  // naming frames things that conflict with window properties that
  // are in Moz but not IE. Since we have some of these, we have to do
  // it the Moz way.
  AtomicString atomicPropertyName = propertyName;
  if (m_frame->tree()->child(atomicPropertyName)) {
    slot.setCustom(this, childFrameGetter);
    return true;
  }
  
  const HashEntry* entry = Lookup::findEntry(&WindowTable, propertyName);
  if (entry) {
    if (entry->attr & Function) {
      switch (entry->value) {
      case Focus:
      case Blur:
      case Close:
        slot.setStaticEntry(this, entry, staticFunctionGetter<WindowFunc>);
        break;
      case ShowModalDialog:
        if (!canShowModalDialog(this))
          return false;
        // fall through
      default:
        if (isSafeScript(exec))
          slot.setStaticEntry(this, entry, staticFunctionGetter<WindowFunc>);
        else
          slot.setUndefined(this);
      } 
    } else
      slot.setStaticEntry(this, entry, staticValueGetter<Window>);
    return true;
  }

  // FIXME: Search the whole frame hierachy somewhere around here.
  // We need to test the correct priority order.
  
  // allow window[1] or parent[1] etc. (#56983)
  bool ok;
  unsigned i = propertyName.toArrayIndex(&ok);
  if (ok && i < m_frame->tree()->childCount()) {
    slot.setCustomIndex(this, i, indexGetter);
    return true;
  }

  // allow shortcuts like 'Image1' instead of document.images.Image1
  Document *doc = m_frame->document();
  if (isSafeScript(exec) && doc && doc->isHTMLDocument()) {
    AtomicString atomicPropertyName = propertyName;
    if (static_cast<HTMLDocument*>(doc)->hasNamedItem(atomicPropertyName) || doc->getElementById(atomicPropertyName)) {
      slot.setCustom(this, namedItemGetter);
      return true;
    }
  }

  if (!isSafeScript(exec)) {
    slot.setUndefined(this);
    return true;
  }

  return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

void Window::put(ExecState* exec, const Identifier &propertyName, JSValue *value, int attr)
{
  // Called by an internal KJS call.
  // If yes, save time and jump directly to JSObject.
  if ((attr != None && attr != DontDelete)
       // Same thing if we have a local override (e.g. "var location")
       || (JSObject::getDirect(propertyName) && isSafeScript(exec))) {
    JSObject::put( exec, propertyName, value, attr );
    return;
  }

  const HashEntry* entry = Lookup::findEntry(&WindowTable, propertyName);
  if (entry) {
    switch(entry->value) {
    case Status:
      m_frame->setJSStatusBarText(value->toString(exec));
      return;
    case DefaultStatus:
      m_frame->setJSDefaultStatusBarText(value->toString(exec));
      return;
    case Location_: {
      Frame* p = Window::retrieveActive(exec)->m_frame;
      if (p) {
        DeprecatedString dstUrl = p->document()->completeURL(DeprecatedString(value->toString(exec)));
        if (!dstUrl.startsWith("javascript:", false) || isSafeScript(exec)) {
          bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
          // We want a new history item if this JS was called via a user gesture
          m_frame->loader()->scheduleLocationChange(dstUrl, p->loader()->outgoingReferrer(), !userGesture, userGesture);
        }
      }
      return;
    }
    case Onabort:
      if (isSafeScript(exec))
        setListener(exec, abortEvent,value);
      return;
    case Onblur:
      if (isSafeScript(exec))
        setListener(exec, blurEvent,value);
      return;
    case Onchange:
      if (isSafeScript(exec))
        setListener(exec, changeEvent,value);
      return;
    case Onclick:
      if (isSafeScript(exec))
        setListener(exec,clickEvent,value);
      return;
    case Ondblclick:
      if (isSafeScript(exec))
        setListener(exec, dblclickEvent,value);
      return;
    case Ondragdrop:
      if (isSafeScript(exec))
        setListener(exec,khtmlDragdropEvent,value);
      return;
    case Onerror:
      if (isSafeScript(exec))
        setListener(exec, errorEvent, value);
      return;
    case Onfocus:
      if (isSafeScript(exec))
        setListener(exec,focusEvent,value);
      return;
    case Onkeydown:
      if (isSafeScript(exec))
        setListener(exec,keydownEvent,value);
      return;
    case Onkeypress:
      if (isSafeScript(exec))
        setListener(exec,keypressEvent,value);
      return;
    case Onkeyup:
      if (isSafeScript(exec))
        setListener(exec,keyupEvent,value);
      return;
    case Onload:
      if (isSafeScript(exec))
        setListener(exec,loadEvent,value);
      return;
    case Onmousedown:
      if (isSafeScript(exec))
        setListener(exec,mousedownEvent,value);
      return;
    case Onmousemove:
      if (isSafeScript(exec))
        setListener(exec,mousemoveEvent,value);
      return;
    case Onmouseout:
      if (isSafeScript(exec))
        setListener(exec,mouseoutEvent,value);
      return;
    case Onmouseover:
      if (isSafeScript(exec))
        setListener(exec,mouseoverEvent,value);
      return;
    case Onmouseup:
      if (isSafeScript(exec))
        setListener(exec,mouseupEvent,value);
      return;
    case OnWindowMouseWheel:
      if (isSafeScript(exec))
        setListener(exec, mousewheelEvent,value);
      return;
    case Onmove:
      if (isSafeScript(exec))
        setListener(exec,khtmlMoveEvent,value);
      return;
    case Onreset:
      if (isSafeScript(exec))
        setListener(exec,resetEvent,value);
      return;
    case Onresize:
      if (isSafeScript(exec))
        setListener(exec,resizeEvent,value);
      return;
    case Onscroll:
      if (isSafeScript(exec))
        setListener(exec,scrollEvent,value);
      return;
    case Onsearch:
        if (isSafeScript(exec))
            setListener(exec,searchEvent,value);
        return;
    case Onselect:
      if (isSafeScript(exec))
        setListener(exec,selectEvent,value);
      return;
    case Onsubmit:
      if (isSafeScript(exec))
        setListener(exec,submitEvent,value);
      return;
    case Onbeforeunload:
      if (isSafeScript(exec))
        setListener(exec, beforeunloadEvent, value);
      return;
    case Onunload:
      if (isSafeScript(exec))
        setListener(exec, unloadEvent, value);
      return;
    case Name:
      if (isSafeScript(exec))
        m_frame->tree()->setName(value->toString(exec));
      return;
    default:
      break;
    }
  }
  if (isSafeScript(exec))
    JSObject::put(exec, propertyName, value, attr);
}

bool Window::toBoolean(ExecState *) const
{
  return m_frame;
}

void Window::scheduleClose()
{
  m_frame->scheduleClose();
}

static bool shouldLoadAsEmptyDocument(const KURL &url)
{
  return url.protocol().lower() == "about" || url.isEmpty();
}

bool Window::isSafeScript(const ScriptInterpreter *origin, const ScriptInterpreter *target)
{
    if (origin == target)
        return true;
        
    Frame* originFrame = origin->frame();
    Frame* targetFrame = target->frame();

    // JS may be attempting to access the "window" object, which should be valid,
    // even if the document hasn't been constructed yet.  If the document doesn't
    // exist yet allow JS to access the window object.
    if (!targetFrame->document())
        return true;

    WebCore::Document *originDocument = originFrame->document();
    WebCore::Document *targetDocument = targetFrame->document();

    if (!targetDocument) {
        return false;
    }

    WebCore::String targetDomain = targetDocument->domain();

    // Always allow local pages to execute any JS.
    if (targetDomain.isNull())
        return true;

    WebCore::String originDomain = originDocument->domain();

    // if this document is being initially loaded as empty by its parent
    // or opener, allow access from any document in the same domain as
    // the parent or opener.
    if (shouldLoadAsEmptyDocument(targetFrame->loader()->url())) {
        Frame* ancestorFrame = targetFrame->loader()->opener() ? targetFrame->loader()->opener() : targetFrame->tree()->parent();
        while (ancestorFrame && shouldLoadAsEmptyDocument(ancestorFrame->loader()->url()))
            ancestorFrame = ancestorFrame->tree()->parent();
        if (ancestorFrame)
            originDomain = ancestorFrame->document()->domain();
    }

    if ( targetDomain == originDomain )
        return true;

    if (Interpreter::shouldPrintExceptions()) {
        printf("Unsafe JavaScript attempt to access frame with URL %s from frame with URL %s. Domains must match.\n", 
             targetDocument->URL().latin1(), originDocument->URL().latin1());
    }
    String message = String::format("Unsafe JavaScript attempt to access frame with URL %s from frame with URL %s. Domains must match.\n", 
                  targetDocument->URL().latin1(), originDocument->URL().latin1());
    if (Page* page = targetFrame->page())
        page->chrome()->addMessageToConsole(message, 1, String()); //fixme: provide a real line number and sourceurl

    return false;
}

bool Window::isSafeScript(ExecState *exec) const
{
  if (!m_frame) // frame deleted ? can't grant access
    return false;
  Frame* activeFrame = static_cast<ScriptInterpreter*>(exec->dynamicInterpreter())->frame();
  if (!activeFrame)
    return false;
  if (activeFrame == m_frame) // Not calling from another frame, no problem.
    return true;

  // JS may be attempting to access the "window" object, which should be valid,
  // even if the document hasn't been constructed yet.  If the document doesn't
  // exist yet allow JS to access the window object.
  if (!m_frame->document())
      return true;

  WebCore::Document* thisDocument = m_frame->document();
  WebCore::Document* actDocument = activeFrame->document();

  WebCore::String actDomain;

  if (!actDocument)
    actDomain = activeFrame->loader()->url().host();
  else
    actDomain = actDocument->domain();
  
  // FIXME: this really should be explicitly checking for the "file:" protocol instead
  // Always allow local pages to execute any JS.
  if (actDomain.isEmpty())
    return true;
  
  WebCore::String thisDomain = thisDocument->domain();

  // if this document is being initially loaded as empty by its parent
  // or opener, allow access from any document in the same domain as
  // the parent or opener.
  if (shouldLoadAsEmptyDocument(m_frame->loader()->url())) {
    Frame* ancestorFrame = m_frame->loader()->opener()
        ? m_frame->loader()->opener() : m_frame->tree()->parent();
    while (ancestorFrame && shouldLoadAsEmptyDocument(ancestorFrame->loader()->url()))
      ancestorFrame = ancestorFrame->tree()->parent();
    if (ancestorFrame)
      thisDomain = ancestorFrame->document()->domain();
  }

  // FIXME: this should check that URL scheme and port match too, probably
  if (actDomain == thisDomain)
    return true;

  if (Interpreter::shouldPrintExceptions()) {
      printf("Unsafe JavaScript attempt to access frame with URL %s from frame with URL %s. Domains must match.\n", 
             thisDocument->URL().latin1(), actDocument->URL().latin1());
  }
  String message = String::format("Unsafe JavaScript attempt to access frame with URL %s from frame with URL %s. Domains must match.\n", 
                  thisDocument->URL().latin1(), actDocument->URL().latin1());
  if (Page* page = m_frame->page())
      page->chrome()->addMessageToConsole(message, 1, String());
  
  return false;
}

void Window::setListener(ExecState *exec, const AtomicString &eventType, JSValue *func)
{
  if (!isSafeScript(exec))
    return;
  WebCore::Document *doc = m_frame->document();
  if (!doc)
    return;

  doc->setHTMLWindowEventListener(eventType, getJSEventListener(func,true));
}

JSValue *Window::getListener(ExecState *exec, const AtomicString &eventType) const
{
  if (!isSafeScript(exec))
    return jsUndefined();
  WebCore::Document *doc = m_frame->document();
  if (!doc)
    return jsUndefined();

  WebCore::EventListener *listener = doc->getHTMLWindowEventListener(eventType);
  if (listener && static_cast<JSEventListener*>(listener)->listenerObj())
    return static_cast<JSEventListener*>(listener)->listenerObj();
  else
    return jsNull();
}

JSEventListener *Window::getJSEventListener(JSValue *val, bool html)
{
  if (!val->isObject())
    return 0;
  JSObject *object = static_cast<JSObject *>(val);

  ListenersMap& listeners = html ? jsHTMLEventListeners : jsEventListeners;
  if (JSEventListener* listener = listeners.get(object))
    return listener;

  // Note that the JSEventListener constructor adds it to our jsEventListeners list
  return new JSEventListener(object, this, html);
}

JSUnprotectedEventListener *Window::getJSUnprotectedEventListener(JSValue *val, bool html)
{
  if (!val->isObject())
    return 0;
  JSObject* object = static_cast<JSObject *>(val);

  UnprotectedListenersMap& listeners = html ? jsUnprotectedHTMLEventListeners : jsUnprotectedEventListeners;
  if (JSUnprotectedEventListener* listener = listeners.get(object))
    return listener;

  // The JSUnprotectedEventListener constructor adds it to our jsUnprotectedEventListeners map.
  return new JSUnprotectedEventListener(object, this, html);
}

void Window::clear()
{
  JSLock lock;

  if (m_returnValueSlot && !*m_returnValueSlot)
    *m_returnValueSlot = getDirect("returnValue");

  clearAllTimeouts();
  clearProperties();
  setPrototype(JSDOMWindowProto::self()); // clear the prototype

  // Now recreate a working global object for the next URL that will use us; but only if we haven't been
  // disconnected yet
  if (m_frame)
    interpreter()->initGlobalObject();

  // there's likely to be lots of garbage now
  Collector::collect();
}

void Window::setCurrentEvent(Event *evt)
{
  m_evt = evt;
}

static void setWindowFeature(const String& keyString, const String& valueString, WindowFeatures& windowFeatures)
{
    int value;
    
    if (valueString.length() == 0 || // listing a key with no value is shorthand for key=yes
        valueString == "yes")
        value = 1;
    else
        value = valueString.toInt();
    
    if (keyString == "left" || keyString == "screenx") {
        windowFeatures.xSet = true;
        windowFeatures.x = value;
    } else if (keyString == "top" || keyString == "screeny") {
        windowFeatures.ySet = true;
        windowFeatures.y = value;
    } else if (keyString == "width" || keyString == "innerwidth") {
        windowFeatures.widthSet = true;
        windowFeatures.width = value;
    } else if (keyString == "height" || keyString == "innerheight") {
        windowFeatures.heightSet = true;
        windowFeatures.height = value;
    } else if (keyString == "menubar")
        windowFeatures.menuBarVisible = value;
    else if (keyString == "toolbar")
        windowFeatures.toolBarVisible = value;
    else if (keyString == "location")
        windowFeatures.locationBarVisible = value;
    else if (keyString == "status")
        windowFeatures.statusBarVisible = value;
    else if (keyString == "resizable")
        windowFeatures.resizable = value;
    else if (keyString == "fullscreen")
        windowFeatures.fullscreen = value;
    else if (keyString == "scrollbars")
        windowFeatures.scrollbarsVisible = value;
}

// Though isspace() considers \t and \v to be whitespace, Win IE doesn't.
static bool isSeparator(::UChar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '=' || c == ',' || c == '\0';
}

static void parseWindowFeatures(const String& features, WindowFeatures& windowFeatures)
{
    /*
     The IE rule is: all features except for channelmode and fullscreen default to YES, but
     if the user specifies a feature string, all features default to NO. (There is no public
     standard that applies to this method.)
     
     <http://msdn.microsoft.com/workshop/author/dhtml/reference/methods/open_0.asp>
     */
    
    windowFeatures.dialog = false;
    windowFeatures.fullscreen = false;
    
    windowFeatures.xSet = false;
    windowFeatures.ySet = false;
    windowFeatures.widthSet = false;
    windowFeatures.heightSet = false;
    
    if (features.length() == 0) {
        windowFeatures.menuBarVisible = true;
        windowFeatures.statusBarVisible = true;
        windowFeatures.toolBarVisible = true;
        windowFeatures.locationBarVisible = true;
        windowFeatures.scrollbarsVisible = true;
        windowFeatures.resizable = true;
        
        return;
    }
    
    windowFeatures.menuBarVisible = false;
    windowFeatures.statusBarVisible = false;
    windowFeatures.toolBarVisible = false;
    windowFeatures.locationBarVisible = false;
    windowFeatures.scrollbarsVisible = false;
    windowFeatures.resizable = false;
    
    // Tread lightly in this code -- it was specifically designed to mimic Win IE's parsing behavior.
    int keyBegin, keyEnd;
    int valueBegin, valueEnd;
    
    int i = 0;
    int length = features.length();
    String buffer = features.lower();
    while (i < length) {
        // skip to first non-separator, but don't skip past the end of the string
        while (isSeparator(buffer[i])) {
            if (i >= length)
                break;
            i++;
        }
        keyBegin = i;
        
        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        keyEnd = i;
        
        // skip to first '=', but don't skip past a ',' or the end of the string
        while (buffer[i] != '=') {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }
        
        // skip to first non-separator, but don't skip past a ',' or the end of the string
        while (isSeparator(buffer[i])) {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }
        valueBegin = i;
        
        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        valueEnd = i;
        
        assert(i <= length);

        String keyString(buffer.substring(keyBegin, keyEnd - keyBegin));
        String valueString(buffer.substring(valueBegin, valueEnd - valueBegin));
        setWindowFeature(keyString, valueString, windowFeatures);
    }
}

static void constrainToVisible(const FloatRect& screen, WindowFeatures& windowFeatures)
{
    windowFeatures.x += screen.x();
    if (windowFeatures.x < screen.x() || windowFeatures.x >= screen.right())
        windowFeatures.x = screen.x(); // only safe choice until size is determined
    
    windowFeatures.y += screen.y();
    if (windowFeatures.y < screen.y() || windowFeatures.y >= screen.bottom())
        windowFeatures.y = screen.y(); // only safe choice until size is determined
    
    if (windowFeatures.height > screen.height()) // should actually check workspace
        windowFeatures.height = screen.height();
    if (windowFeatures.height < 100)
        windowFeatures.height = 100;
    
    if (windowFeatures.width > screen.width()) // should actually check workspace
        windowFeatures.width = screen.width();
    if (windowFeatures.width < 100)
        windowFeatures.width = 100;
}

JSValue *WindowFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  if (!thisObj->inherits(&Window::info))
    return throwError(exec, TypeError);
  Window *window = static_cast<Window *>(thisObj);
  Frame *frame = window->m_frame;
  if (!frame)
    return jsUndefined();

  FrameView *widget = frame->view();
  Page* page = frame->page();
  JSValue *v = args[0];
  UString s = v->toString(exec);
  String str = s;
  String str2;

  switch (id) {
  case Window::Alert:
    if (frame && frame->document())
      frame->document()->updateRendering();
    exec->dynamicInterpreter()->pauseTimeoutCheck();
    frame->runJavaScriptAlert(str);
    exec->dynamicInterpreter()->resumeTimeoutCheck();
    return jsUndefined();
  case Window::AToB:
  case Window::BToA: {
    if (args.size() < 1)
        return throwError(exec, SyntaxError, "Not enough arguments");
    if (v->isNull())
        return jsString();
    if (!s.is8Bit()) {
        setDOMException(exec, INVALID_CHARACTER_ERR);
        return jsUndefined();
    }
    
    Vector<char> in(s.size());
    for (int i = 0; i < s.size(); ++i)
        in[i] = static_cast<char>(s.data()[i].unicode());
    Vector<char> out;

    if (id == Window::AToB) {
        if (!base64Decode(in, out))
            return throwError(exec, GeneralError, "Cannot decode base64");
    } else
        base64Encode(in, out);
    
    return jsString(String(out.data(), out.size()));
  }
  case Window::Confirm: {
    if (frame && frame->document())
      frame->document()->updateRendering();
    exec->dynamicInterpreter()->pauseTimeoutCheck();
    bool result = frame->runJavaScriptConfirm(str);
    exec->dynamicInterpreter()->resumeTimeoutCheck();
    return jsBoolean(result);
  }
  case Window::Prompt:
  {
    if (frame && frame->document())
      frame->document()->updateRendering();
    String message = args.size() >= 2 ? args[1]->toString(exec) : UString();
    bool ok = frame->runJavaScriptPrompt(str, message, str2);
    if (ok)
        return jsString(str2);
    else
        return jsNull();
  }
  case Window::Open:
  {
      AtomicString frameName = args[1]->isUndefinedOrNull()
        ? "_blank" : AtomicString(args[1]->toString(exec));
      // Because FrameTree::find() returns true for empty strings, we must check for empty framenames.
      // Otherwise, illegitimate window.open() calls with no name will pass right through the popup blocker.
      if (!allowPopUp(exec, window) && (frameName.isEmpty() || !frame->tree()->find(frameName)))
          return jsUndefined();
      
      WindowFeatures windowFeatures;
      String features = args[2]->isUndefinedOrNull() ? UString() : args[2]->toString(exec);
      parseWindowFeatures(features, windowFeatures);
      constrainToVisible(screenRect(page->mainFrame()->view()), windowFeatures);
      
      // prepare arguments
      KURL url;
      Frame* activeFrame = Window::retrieveActive(exec)->m_frame;
      if (!str.isEmpty() && activeFrame)
          url = activeFrame->document()->completeURL(str.deprecatedString());

      ResourceRequest resRequest(url);
      FrameLoadRequest frameRequest(resRequest, frameName);
      if (frameRequest.frameName() == "_top") {
          while (frame->tree()->parent())
              frame = frame->tree()->parent();
          
          const Window* window = Window::retrieveWindow(frame);
          if (!url.url().startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
              bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
              frame->loader()->scheduleLocationChange(url.url(), activeFrame->loader()->outgoingReferrer(), false/*don't lock history*/, userGesture);
          }
          return Window::retrieve(frame);
      }
      if (frameRequest.frameName() == "_parent") {
          if (frame->tree()->parent())
              frame = frame->tree()->parent();
          
          const Window* window = Window::retrieveWindow(frame);
          if (!url.url().startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
              bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
              frame->loader()->scheduleLocationChange(url.url(), activeFrame->loader()->outgoingReferrer(), false/*don't lock history*/, userGesture);
          }
          return Window::retrieve(frame);
      }
      
      // request window (new or existing if framename is set)
      frameRequest.resourceRequest().setHTTPReferrer(activeFrame->loader()->outgoingReferrer());
      Frame* newFrame = frame->loader()->createWindow(frameRequest, windowFeatures);
      if (!newFrame)
          return jsUndefined();
      newFrame->loader()->setOpener(frame);
      newFrame->loader()->setOpenedByJavaScript();
      
      if (!newFrame->document()) {
          Document* oldDoc = frame->document();
          if (oldDoc && oldDoc->baseURL() != 0)
              newFrame->loader()->begin(oldDoc->baseURL());
          else
              newFrame->loader()->begin();
          newFrame->loader()->write("<HTML><BODY>");
          newFrame->loader()->end();          
          if (oldDoc) {
              newFrame->document()->setDomain(oldDoc->domain(), true);
              newFrame->document()->setBaseURL(oldDoc->baseURL());
          }
      }
      if (!url.isEmpty()) {
          const Window* window = Window::retrieveWindow(newFrame);
          if (!url.url().startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
              bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
              newFrame->loader()->scheduleLocationChange(url.url(), activeFrame->loader()->outgoingReferrer(), false, userGesture);
          }
      }
      return Window::retrieve(newFrame); // global object
  }
  case Window::Print:
    frame->print();
    return jsUndefined();
  case Window::ScrollBy:
    window->updateLayout();
    if(args.size() >= 2 && widget)
      widget->scrollBy(args[0]->toInt32(exec), args[1]->toInt32(exec));
    return jsUndefined();
  case Window::Scroll:
  case Window::ScrollTo:
    window->updateLayout();
    if (args.size() >= 2 && widget)
      widget->setContentsPos(args[0]->toInt32(exec), args[1]->toInt32(exec));
    return jsUndefined();
  case Window::MoveBy:
    if (args.size() >= 2 && page) {
      FloatRect r = page->chrome()->windowRect();
      r.move(args[0]->toNumber(exec), args[1]->toNumber(exec));
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      if (screenRect(page->mainFrame()->view()).contains(r))
        page->chrome()->setWindowRect(r);
    }
    return jsUndefined();
  case Window::MoveTo:
    if (args.size() >= 2 && page) {
      FloatRect r = page->chrome()->windowRect();
      FloatRect sr = screenRect(page->mainFrame()->view());
      r.setLocation(sr.location());
      r.move(args[0]->toNumber(exec), args[1]->toNumber(exec));
      // Security check (the spec talks about UniversalBrowserWrite to disable this check...)
      if (sr.contains(r))
        page->chrome()->setWindowRect(r);
    }
    return jsUndefined();
  case Window::ResizeBy:
    if (args.size() >= 2 && page) {
      FloatRect r = page->chrome()->windowRect();
      FloatSize dest = r.size() + FloatSize(args[0]->toNumber(exec), args[1]->toNumber(exec));
      FloatRect sg = screenRect(page->mainFrame()->view());
      // Security check: within desktop limits and bigger than 100x100 (per spec)
      if (r.x() + dest.width() <= sg.right() && r.y() + dest.height() <= sg.bottom()
           && dest.width() >= 100 && dest.height() >= 100)
        page->chrome()->setWindowRect(FloatRect(r.location(), dest));
    }
    return jsUndefined();
  case Window::ResizeTo:
    if (args.size() >= 2 && page) {
      FloatRect r = page->chrome()->windowRect();
      FloatSize dest = FloatSize(args[0]->toNumber(exec), args[1]->toNumber(exec));
      FloatRect sg = screenRect(page->mainFrame()->view());
      // Security check: within desktop limits and bigger than 100x100 (per spec)
      if (r.x() + dest.width() <= sg.right() && r.y() + dest.height() <= sg.bottom() &&
           dest.width() >= 100 && dest.height() >= 100)
        page->chrome()->setWindowRect(FloatRect(r.location(), dest));
    }
    return jsUndefined();
  case Window::SetTimeout:
    if (!window->isSafeScript(exec))
        return jsUndefined();
    if (args.size() >= 2 && v->isString()) {
      int i = args[1]->toInt32(exec);
      int r = (const_cast<Window*>(window))->installTimeout(s, i, true /*single shot*/);
      return jsNumber(r);
    }
    else if (args.size() >= 2 && v->isObject() && static_cast<JSObject *>(v)->implementsCall()) {
      JSValue *func = args[0];
      int i = args[1]->toInt32(exec);

      // All arguments after the second should go to the function
      // FIXME: could be more efficient
      List funcArgs = args.copyTail().copyTail();

      int r = (const_cast<Window*>(window))->installTimeout(func, funcArgs, i, true /*single shot*/);
      return jsNumber(r);
    }
    else
      return jsUndefined();
  case Window::SetInterval:
    if (!window->isSafeScript(exec))
        return jsUndefined();
    if (args.size() >= 2 && v->isString()) {
      int i = args[1]->toInt32(exec);
      int r = (const_cast<Window*>(window))->installTimeout(s, i, false);
      return jsNumber(r);
    }
    else if (args.size() >= 2 && v->isObject() && static_cast<JSObject *>(v)->implementsCall()) {
      JSValue *func = args[0];
      int i = args[1]->toInt32(exec);

      // All arguments after the second should go to the function
      // FIXME: could be more efficient
      List funcArgs = args.copyTail().copyTail();

      int r = (const_cast<Window*>(window))->installTimeout(func, funcArgs, i, false);
      return jsNumber(r);
    }
    else
      return jsUndefined();
  case Window::ClearTimeout:
  case Window::ClearInterval:
    if (!window->isSafeScript(exec))
        return jsUndefined();
    (const_cast<Window*>(window))->clearTimeout(v->toInt32(exec));
    return jsUndefined();
  case Window::Focus:
    frame->focusWindow();
    return jsUndefined();
  case Window::GetSelection:
    if (!window->isSafeScript(exec))
        return jsUndefined();
    return window->selection();
  case Window::Blur:
    frame->unfocusWindow();
    return jsUndefined();
  case Window::Close:
    // Do not close windows that have history unless they were opened by JavaScript.
    if (frame->loader()->openedByJavaScript() || frame->loader()->getHistoryLength() <= 1)
      const_cast<Window*>(window)->scheduleClose();
    return jsUndefined();
  case Window::CaptureEvents:
  case Window::ReleaseEvents:
    // If anyone implements these, they need the safe script security check.
    if (!window->isSafeScript(exec))
        return jsUndefined();
    // Not implemented.
    return jsUndefined();
  case Window::AddEventListener:
        if (!window->isSafeScript(exec))
            return jsUndefined();
        if (JSEventListener *listener = Window::retrieveActive(exec)->getJSEventListener(args[1]))
            if (Document *doc = frame->document())
                doc->addWindowEventListener(AtomicString(args[0]->toString(exec)), listener, args[2]->toBoolean(exec));
        return jsUndefined();
  case Window::RemoveEventListener:
        if (!window->isSafeScript(exec))
            return jsUndefined();
        if (JSEventListener *listener = Window::retrieveActive(exec)->getJSEventListener(args[1]))
            if (Document *doc = frame->document())
                doc->removeWindowEventListener(AtomicString(args[0]->toString(exec)), listener, args[2]->toBoolean(exec));
        return jsUndefined();
  case Window::ShowModalDialog: {
    exec->dynamicInterpreter()->pauseTimeoutCheck();
    JSValue* result = showModalDialog(exec, window, args);
    exec->dynamicInterpreter()->resumeTimeoutCheck();      
    return result;
  }
  }
  return jsUndefined();
}

void Window::updateLayout() const
{
  WebCore::Document* docimpl = m_frame->document();
  if (docimpl)
    docimpl->updateLayoutIgnorePendingStylesheets();
}

////////////////////// ScheduledAction ////////////////////////

void ScheduledAction::execute(Window *window)
{
    if (!window->m_frame || !window->m_frame->scriptProxy())
        return;

    ScriptInterpreter *interpreter = window->m_frame->scriptProxy()->interpreter();

    interpreter->setProcessingTimerCallback(true);
  
    if (JSValue* func = m_func.get()) {
        if (func->isObject() && static_cast<JSObject *>(func)->implementsCall()) {
            ExecState *exec = interpreter->globalExec();
            ASSERT(window == interpreter->globalObject());
            JSLock lock;
            interpreter->startTimeoutCheck();
            static_cast<JSObject *>(func)->call(exec, window, m_args);
            interpreter->stopTimeoutCheck();
            if (exec->hadException()) {
                JSObject* exception = exec->exception()->toObject(exec);
                exec->clearException();
                String message = exception->get(exec, messagePropertyName)->toString(exec);
                int lineNumber = exception->get(exec, "line")->toInt32(exec);
                if (Interpreter::shouldPrintExceptions())
                    printf("(timer):%s\n", message.utf8().data());
                if (Page* page = window->m_frame->page())
                    page->chrome()->addMessageToConsole(message, lineNumber, String());
            }
        }
    } else
        window->m_frame->loader()->executeScript(0, m_code);
  
    // Update our document's rendering following the execution of the timeout callback.
    // FIXME: Why? Why not other documents, for example?
    Document *doc = window->m_frame->document();
    if (doc)
        doc->updateRendering();
  
    interpreter->setProcessingTimerCallback(false);
}

////////////////////// timeouts ////////////////////////

void Window::clearAllTimeouts()
{
    deleteAllValues(m_timeouts);
    m_timeouts.clear();
}

int Window::installTimeout(ScheduledAction* a, int t, bool singleShot)
{
    int timeoutId = ++lastUsedTimeoutId;
    int nestLevel = timerNestingLevel + 1;
    DOMWindowTimer* timer = new DOMWindowTimer(timeoutId, nestLevel, this, a);
    ASSERT(!m_timeouts.get(timeoutId));
    m_timeouts.set(timeoutId, timer);
    // Use a minimum interval of 10 ms to match other browsers, but only once we've
    // nested enough to notice that we're repeating.
    // Faster timers might be "better", but they're incompatible.
    double interval = max(0.001, t * 0.001);
    if (interval < cMinimumTimerInterval && nestLevel >= cMaxTimerNestingLevel)
        interval = cMinimumTimerInterval;
    if (singleShot)
        timer->startOneShot(interval);
    else
        timer->startRepeating(interval);
    return timeoutId;
}

int Window::installTimeout(const UString& handler, int t, bool singleShot)
{
    return installTimeout(new ScheduledAction(handler), t, singleShot);
}

int Window::installTimeout(JSValue* func, const List& args, int t, bool singleShot)
{
    return installTimeout(new ScheduledAction(func, args), t, singleShot);
}

PausedTimeouts* Window::pauseTimeouts()
{
    size_t count = m_timeouts.size();
    if (count == 0)
        return 0;

    PausedTimeout* t = new PausedTimeout [count];
    PausedTimeouts* result = new PausedTimeouts(t, count);

    TimeoutsMap::iterator it = m_timeouts.begin();
    for (size_t i = 0; i != count; ++i, ++it) {
        int timeoutId = it->first;
        DOMWindowTimer* timer = it->second;
        t[i].timeoutId = timeoutId;
        t[i].nestingLevel = timer->nestingLevel();
        t[i].nextFireInterval = timer->nextFireInterval();
        t[i].repeatInterval = timer->repeatInterval();
        t[i].action = timer->takeAction();
    }
    ASSERT(it == m_timeouts.end());

    deleteAllValues(m_timeouts);
    m_timeouts.clear();

    return result;
}

void Window::resumeTimeouts(PausedTimeouts* timeouts)
{
    if (!timeouts)
        return;
    size_t count = timeouts->numTimeouts();
    PausedTimeout* array = timeouts->takeTimeouts();
    for (size_t i = 0; i != count; ++i) {
        int timeoutId = array[i].timeoutId;
        DOMWindowTimer* timer = new DOMWindowTimer(timeoutId, array[i].nestingLevel, this, array[i].action);
        m_timeouts.set(timeoutId, timer);
        timer->start(array[i].nextFireInterval, array[i].repeatInterval);
    }
    delete [] array;
}

void Window::clearTimeout(int timeoutId, bool delAction)
{
    TimeoutsMap::iterator it = m_timeouts.find(timeoutId);
    if (it == m_timeouts.end())
        return;
    DOMWindowTimer* timer = it->second;
    m_timeouts.remove(it);
    delete timer;
}

void Window::timerFired(DOMWindowTimer* timer)
{
    // Simple case for non-one-shot timers.
    if (timer->isActive()) {
        int timeoutId = timer->timeoutId();

        timer->action()->execute(this);
        if (m_timeouts.contains(timeoutId) && timer->repeatInterval() && timer->repeatInterval() < cMinimumTimerInterval) {
            timer->setNestingLevel(timer->nestingLevel() + 1);
            if (timer->nestingLevel() >= cMaxTimerNestingLevel)
                timer->augmentRepeatInterval(cMinimumTimerInterval - timer->repeatInterval());
        }
        return;
    }

    // Delete timer before executing the action for one-shot timers.
    ScheduledAction* action = timer->takeAction();
    m_timeouts.remove(timer->timeoutId());
    delete timer;
    action->execute(this);
    delete action;
}

void Window::disconnectFrame()
{
    clearAllTimeouts();
    m_frame = 0;
    if (loc)
        loc->m_frame = 0;
    if (m_selection)
        m_selection->m_frame = 0;
    if (m_locationbar)
        m_locationbar->m_frame = 0;
    if (m_menubar)
        m_menubar->m_frame = 0;
    if (m_personalbar)
        m_personalbar->m_frame = 0;
    if (m_statusbar)
        m_statusbar->m_frame = 0;
    if (m_toolbar)
        m_toolbar->m_frame = 0;
    if (m_scrollbars)
        m_scrollbars->m_frame = 0;
    if (frames)
        frames->disconnectFrame();
    if (history)
        history->disconnectFrame();
}

const ClassInfo FrameArray::info = { "FrameArray", 0, &FrameArrayTable, 0 };

/*
@begin FrameArrayTable 2
length          FrameArray::Length      DontDelete|ReadOnly
location        FrameArray::Location    DontDelete|ReadOnly
@end
*/

JSValue *FrameArray::getValueProperty(ExecState *exec, int token)
{
  switch (token) {
  case Length:
    return jsNumber(m_frame->tree()->childCount());
  case Location:
    // non-standard property, but works in NS and IE
    if (JSObject *obj = Window::retrieveWindow(m_frame))
      return obj->get(exec, "location");
    return jsUndefined();
  default:
    ASSERT(0);
    return jsUndefined();
  }
}

JSValue* FrameArray::indexGetter(ExecState*, JSObject*, const Identifier&, const PropertySlot& slot)
{
    return Window::retrieve(static_cast<FrameArray*>(slot.slotBase())->m_frame->tree()->child(slot.index()));
}
  
JSValue* FrameArray::nameGetter(ExecState*, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
    return Window::retrieve(static_cast<FrameArray*>(slot.slotBase())->m_frame->tree()->child(AtomicString(propertyName)));
}

bool FrameArray::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (!m_frame) {
    slot.setUndefined(this);
    return true;
  }

  const HashEntry* entry = Lookup::findEntry(&FrameArrayTable, propertyName);
  if (entry) {
    slot.setStaticEntry(this, entry, staticValueGetter<FrameArray>);
    return true;
  }

  // check for the name or number
  if (m_frame->tree()->child(propertyName)) {
    slot.setCustom(this, nameGetter);
    return true;
  }

  bool ok;
  unsigned i = propertyName.toArrayIndex(&ok);
  if (ok && i < m_frame->tree()->childCount()) {
    slot.setCustomIndex(this, i, indexGetter);
    return true;
  }

  return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

UString FrameArray::toString(ExecState *) const
{
  return "[object FrameArray]";
}

////////////////////// Location Object ////////////////////////

const ClassInfo Location::info = { "Location", 0, &LocationTable, 0 };
/*
@begin LocationTable 12
  assign        Location::Assign        DontDelete|Function 1
  hash          Location::Hash          DontDelete
  host          Location::Host          DontDelete
  hostname      Location::Hostname      DontDelete
  href          Location::Href          DontDelete
  pathname      Location::Pathname      DontDelete
  port          Location::Port          DontDelete
  protocol      Location::Protocol      DontDelete
  search        Location::Search        DontDelete
  toString      Location::ToString      DontDelete|Function 0
  replace       Location::Replace       DontDelete|Function 1
  reload        Location::Reload        DontDelete|Function 0
@end
*/
KJS_IMPLEMENT_PROTOFUNC(LocationFunc)
Location::Location(Frame *p) : m_frame(p)
{
}

JSValue *Location::getValueProperty(ExecState *exec, int token) const
{
  KURL url = m_frame->loader()->url();
  switch (token) {
  case Hash:
    return jsString(url.ref().isNull() ? "" : "#" + url.ref());
  case Host: {
    // Note: this is the IE spec. The NS spec swaps the two, it says
    // "The hostname property is the concatenation of the host and port properties, separated by a colon."
    // Bleh.
    UString str = url.host();
    if (url.port())
        str += ":" + String::number((int)url.port());
    return jsString(str);
  }
  case Hostname:
    return jsString(url.host());
  case Href:
    if (!url.hasPath())
      return jsString(url.prettyURL() + "/");
    return jsString(url.prettyURL());
  case Pathname:
    return jsString(url.path().isEmpty() ? "/" : url.path());
  case Port:
    return jsString(url.port() ? String::number((int)url.port()) : "");
  case Protocol:
    return jsString(url.protocol() + ":");
  case Search:
    return jsString(url.query());
  default:
    ASSERT(0);
    return jsUndefined();
  }
}

bool Location::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot) 
{
  if (!m_frame)
    return false;
  
  const Window* window = Window::retrieveWindow(m_frame);
  if (!window || !window->isSafeScript(exec)) {
      slot.setUndefined(this);
      return true;
  }

  return getStaticPropertySlot<LocationFunc, Location, JSObject>(exec, &LocationTable, this, propertyName, slot);
}

void Location::put(ExecState *exec, const Identifier &p, JSValue *v, int attr)
{
  if (!m_frame)
    return;

  DeprecatedString str = v->toString(exec);
  KURL url = m_frame->loader()->url();
  const HashEntry *entry = Lookup::findEntry(&LocationTable, p);
  if (entry)
    switch (entry->value) {
    case Href: {
      Frame* p = Window::retrieveActive(exec)->frame();
      if ( p )
        url = p->document()->completeURL( str );
      else
        url = str;
      break;
    }
    case Hash: {
      if (str.startsWith("#"))
        str = str.mid(1);

      if (url.ref() == str)
          return;

      url.setRef(str);
      break;
    }
    case Host: {
      DeprecatedString host = str.left(str.find(":"));
      DeprecatedString port = str.mid(str.find(":")+1);
      url.setHost(host);
      url.setPort(port.toUInt());
      break;
    }
    case Hostname:
      url.setHost(str);
      break;
    case Pathname:
      url.setPath(str);
      break;
    case Port:
      url.setPort(str.toUInt());
      break;
    case Protocol:
      url.setProtocol(str);
      break;
    case Search:
      url.setQuery(str);
      break;
    }
  else {
    JSObject::put(exec, p, v, attr);
    return;
  }

  const Window* window = Window::retrieveWindow(m_frame);
  Frame* activeFrame = Window::retrieveActive(exec)->frame();
  if (!url.url().startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
    bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
    // We want a new history item if this JS was called via a user gesture
    m_frame->loader()->scheduleLocationChange(url.url(), activeFrame->loader()->outgoingReferrer(), !userGesture, userGesture);
  }
}

JSValue *Location::toPrimitive(ExecState *exec, JSType) const
{
  return jsString(toString(exec));
}

UString Location::toString(ExecState* exec) const
{
  if (!m_frame || !Window::retrieveWindow(m_frame)->isSafeScript(exec))
    return UString();

  if (!m_frame->loader()->url().hasPath())
    return m_frame->loader()->url().prettyURL()+"/";
  return m_frame->loader()->url().prettyURL();
}

JSValue *LocationFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  if (!thisObj->inherits(&Location::info))
    return throwError(exec, TypeError);
  Location *location = static_cast<Location *>(thisObj);
  Frame *frame = location->frame();
  if (frame) {
      
    Window* window = Window::retrieveWindow(frame);
    if (!window->isSafeScript(exec) && id != Location::Replace)
        return jsUndefined();
      
    switch (id) {
    case Location::Replace:
    {
      DeprecatedString str = args[0]->toString(exec);
      Frame* p = Window::retrieveActive(exec)->frame();
      if ( p ) {
        const Window* window = Window::retrieveWindow(frame);
        if (!str.startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
          bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
          frame->loader()->scheduleLocationChange(p->document()->completeURL(str), p->loader()->outgoingReferrer(), true /*lock history*/, userGesture);
        }
      }
      break;
    }
    case Location::Reload:
    {
      const Window* window = Window::retrieveWindow(frame);
      if (!frame->loader()->url().url().startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
        bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
        frame->loader()->scheduleRefresh(userGesture);
      }
      break;
    }
    case Location::Assign:
    {
        Frame *p = Window::retrieveActive(exec)->frame();
        if (p) {
            const Window *window = Window::retrieveWindow(frame);
            DeprecatedString dstUrl = p->document()->completeURL(DeprecatedString(args[0]->toString(exec)));
            if (!dstUrl.startsWith("javascript:", false) || (window && window->isSafeScript(exec))) {
                bool userGesture = static_cast<ScriptInterpreter *>(exec->dynamicInterpreter())->wasRunByUserGesture();
                // We want a new history item if this JS was called via a user gesture
                frame->loader()->scheduleLocationChange(dstUrl, p->loader()->outgoingReferrer(), !userGesture, userGesture);
            }
        }
        break;
    }
    case Location::ToString:
      return jsString(location->toString(exec));
    }
  }
  return jsUndefined();
}

////////////////////// Selection Object ////////////////////////

const ClassInfo Selection::info = { "Selection", 0, &SelectionTable, 0 };
/*
@begin SelectionTable 19
  anchorNode                Selection::AnchorNode               DontDelete|ReadOnly
  anchorOffset              Selection::AnchorOffset             DontDelete|ReadOnly
  focusNode                 Selection::FocusNode                DontDelete|ReadOnly
  focusOffset               Selection::FocusOffset              DontDelete|ReadOnly
  baseNode                  Selection::BaseNode                 DontDelete|ReadOnly
  baseOffset                Selection::BaseOffset               DontDelete|ReadOnly
  extentNode                Selection::ExtentNode               DontDelete|ReadOnly
  extentOffset              Selection::ExtentOffset             DontDelete|ReadOnly
  isCollapsed               Selection::IsCollapsed              DontDelete|ReadOnly
  type                      Selection::_Type                    DontDelete|ReadOnly
  rangeCount                Selection::RangeCount               DontDelete|ReadOnly
  toString                  Selection::ToString                 DontDelete|Function 0
  collapse                  Selection::Collapse                 DontDelete|Function 2
  collapseToEnd             Selection::CollapseToEnd            DontDelete|Function 0
  collapseToStart           Selection::CollapseToStart          DontDelete|Function 0
  empty                     Selection::Empty                    DontDelete|Function 0
  setBaseAndExtent          Selection::SetBaseAndExtent         DontDelete|Function 4
  setPosition               Selection::SetPosition              DontDelete|Function 2
  modify                    Selection::Modify                   DontDelete|Function 3
  getRangeAt                Selection::GetRangeAt               DontDelete|Function 1
  removeAllRanges           Selection::RemoveAllRanges          DontDelete|Function 0
  addRange                  Selection::AddRange                 DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(SelectionFunc)
Selection::Selection(Frame *p) : m_frame(p)
{
}

JSValue *Selection::getValueProperty(ExecState *exec, int token) const
{
    SelectionController* s = m_frame->selectionController();
    const Window* window = Window::retrieveWindow(m_frame);
    if (!window)
        return jsUndefined();
        
    switch (token) {
    case AnchorNode:
        return toJS(exec, s->anchorNode());
    case BaseNode:
        return toJS(exec, s->baseNode());
    case AnchorOffset:
        return jsNumber(s->anchorOffset());
    case BaseOffset:
        return jsNumber(s->baseOffset());
    case FocusNode:
        return toJS(exec, s->focusNode());
    case ExtentNode:
        return toJS(exec, s->extentNode());
    case FocusOffset:
        return jsNumber(s->focusOffset());
    case ExtentOffset:
        return jsNumber(s->extentOffset());
    case IsCollapsed:
        return jsBoolean(s->isCollapsed());
    case _Type:
        return jsString(s->type());
    case RangeCount:
        return jsNumber(s->rangeCount());
    default:
        ASSERT(0);
        return jsUndefined();
    }
}

bool Selection::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (!m_frame)
      return false;

  return getStaticPropertySlot<SelectionFunc, Selection, JSObject>(exec, &SelectionTable, this, propertyName, slot);
}

JSValue *Selection::toPrimitive(ExecState *exec, JSType) const
{
  return jsString(toString(exec));
}

UString Selection::toString(ExecState *) const
{
    return UString(m_frame->selectionController()->toString());
}

JSValue *SelectionFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
    if (!thisObj->inherits(&Selection::info))
        return throwError(exec, TypeError);
    Selection *selection = static_cast<Selection *>(thisObj);
    Frame *frame = selection->frame();
    if (frame) {
        SelectionController* s = frame->selectionController();
        
        switch (id) {
            case Selection::Collapse:
                s->collapse(toNode(args[0]), args[1]->toInt32(exec));
                break;
            case Selection::CollapseToEnd:
                s->collapseToEnd();
                break;
            case Selection::CollapseToStart:
                s->collapseToStart();
                break;
            case Selection::Empty:
                s->empty();
                break;
            case Selection::SetBaseAndExtent:
                s->setBaseAndExtent(toNode(args[0]), args[1]->toInt32(exec), toNode(args[2]), args[3]->toInt32(exec));
                break;
            case Selection::SetPosition:
                s->setPosition(toNode(args[0]), args[1]->toInt32(exec));
                break;
            case Selection::Modify:
                s->modify(args[0]->toString(exec), args[1]->toString(exec), args[2]->toString(exec));
                break;
            case Selection::GetRangeAt:
                return toJS(exec, s->getRangeAt(args[0]->toInt32(exec)).get());
            case Selection::RemoveAllRanges:
                s->removeAllRanges();
                break;
            case Selection::AddRange:
                s->addRange(toRange(args[0]));
                break;
            case Selection::ToString:
                return jsString(s->toString());
        }
    }

    return jsUndefined();
}

////////////////////// BarInfo Object ////////////////////////

const ClassInfo BarInfo::info = { "BarInfo", 0, &BarInfoTable, 0 };
/*
@begin BarInfoTable 1
  visible                BarInfo::Visible                        DontDelete|ReadOnly
@end
*/
BarInfo::BarInfo(ExecState *exec, Frame *f, Type barType) 
  : m_frame(f)
  , m_type(barType)
{
  setPrototype(exec->lexicalInterpreter()->builtinObjectPrototype());
}

JSValue *BarInfo::getValueProperty(ExecState *exec, int token) const
{
    ASSERT(token == Visible);
    switch (m_type) {
    case Locationbar:
        return jsBoolean(m_frame->page()->chrome()->toolbarsVisible());
    case Toolbar:
        return jsBoolean(m_frame->page()->chrome()->toolbarsVisible());
    case Personalbar:
        return jsBoolean(m_frame->page()->chrome()->toolbarsVisible());
    case Menubar: 
        return jsBoolean(m_frame->page()->chrome()->menubarVisible());
    case Scrollbars: 
        return jsBoolean(m_frame->page()->chrome()->scrollbarsVisible());
    case Statusbar:
        return jsBoolean(m_frame->page()->chrome()->statusbarVisible());
    default:
        return jsBoolean(false);
    }
}

bool BarInfo::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  if (!m_frame)
    return false;
  
  return getStaticValueSlot<BarInfo, JSObject>(exec, &BarInfoTable, this, propertyName, slot);
}

////////////////////// History Object ////////////////////////

const ClassInfo History::info = { "History", 0, &HistoryTable, 0 };
/*
@begin HistoryTable 4
  length        History::Length         DontDelete|ReadOnly
  back          History::Back           DontDelete|Function 0
  forward       History::Forward        DontDelete|Function 0
  go            History::Go             DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(HistoryFunc)

bool History::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
  return getStaticPropertySlot<HistoryFunc, History, JSObject>(exec, &HistoryTable, this, propertyName, slot);
}

JSValue *History::getValueProperty(ExecState *, int token) const
{
    ASSERT(token == Length);
    return m_frame ? jsNumber(m_frame->loader()->getHistoryLength()) : jsNumber(0);
}

UString History::toString(ExecState *exec) const
{
  return "[object History]";
}

JSValue *HistoryFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  if (!thisObj->inherits(&History::info))
    return throwError(exec, TypeError);
  History *history = static_cast<History *>(thisObj);

  int steps;
  switch (id) {
  case History::Back:
    steps = -1;
    break;
  case History::Forward:
    steps = 1;
    break;
  case History::Go:
    steps = args[0]->toInt32(exec);
    break;
  default:
    return jsUndefined();
  }

  if (Frame* frame = history->m_frame)
      frame->loader()->scheduleHistoryNavigation(steps);
  return jsUndefined();
}

/////////////////////////////////////////////////////////////////////////////

PausedTimeouts::~PausedTimeouts()
{
    PausedTimeout *array = m_array;
    if (!array)
        return;
    size_t count = m_length;
    for (size_t i = 0; i != count; ++i)
        delete array[i].action;
    delete [] array;
}

void DOMWindowTimer::fired()
{
    timerNestingLevel = m_nestingLevel;
    m_object->timerFired(this);
    timerNestingLevel = 0;
}

} // namespace KJS

using namespace KJS;

namespace WebCore {

JSValue* toJS(ExecState*, DOMWindow* domWindow)
{
    if (!domWindow)
        return jsNull();
    Frame* frame = domWindow->frame();
    if (!frame)
        return jsNull();
    return Window::retrieve(frame);
}

DOMWindow* toDOMWindow(JSValue* val)
{
    return val->isObject(&JSDOMWindow::info) ? static_cast<JSDOMWindow*>(val)->impl() : 0;
}
    
} // namespace WebCore
