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

#import <JavaScriptCore/npruntime.h>
#import <JavaVM/jni.h>
#import <WebCore/WebCoreKeyboardAccess.h>

#ifdef __cplusplus

namespace WebCore {
    class FrameMac;
    class Element;
}

typedef WebCore::FrameMac WebCoreMacFrame;
typedef WebCore::Element WebCoreElement;

#else

@class WebCoreMacFrame;
@class WebCoreElement;

#endif

@class DOMCSSStyleDeclaration;
@class DOMDocument;
@class DOMDocumentFragment;
@class DOMElement;
@class DOMHTMLElement;
@class DOMHTMLInputElement;
@class DOMHTMLTextAreaElement;
@class DOMNode;
@class DOMRange;
@class WebCorePageBridge;
@class WebCoreSettings;
@class WebScriptObject;
@class WebView;

@protocol WebCoreRenderTreeCopier;
@protocol WebCoreResourceHandle;
@protocol WebCoreResourceLoader;

extern NSString *WebCorePageCacheStateKey;

typedef enum {
    WebCoreDeviceScreen,
    WebCoreDevicePrinter
} WebCoreDeviceType;

typedef enum {
    WebSelectionStateNone,
    WebSelectionStateCaret,
    WebSelectionStateRange
} WebSelectionState;

typedef enum {
    WebSelectByMoving,
    WebSelectByExtending
} WebSelectionAlteration;

typedef enum {
    WebBridgeSelectForward,
    WebBridgeSelectBackward,
    WebBridgeSelectRight,
    WebBridgeSelectLeft
} WebBridgeSelectionDirection;

typedef enum {
    WebBridgeSelectByCharacter,
    WebBridgeSelectByWord,
    WebBridgeSelectBySentence,
    WebBridgeSelectByLine,
    WebBridgeSelectByParagraph,
    WebBridgeSelectToSentenceBoundary,
    WebBridgeSelectToLineBoundary,
    WebBridgeSelectToParagraphBoundary,
    WebBridgeSelectToDocumentBoundary
} WebBridgeSelectionGranularity;

typedef enum {
    WebScrollUp,
    WebScrollDown,
    WebScrollLeft,
    WebScrollRight
} WebScrollDirection;

typedef enum {
    WebScrollLine,
    WebScrollPage,
    WebScrollDocument,
    WebScrollWheel
} WebScrollGranularity;

typedef enum {
    WebUndoActionUnspecified,
    WebUndoActionSetColor,
    WebUndoActionSetBackgroundColor,
    WebUndoActionTurnOffKerning,
    WebUndoActionTightenKerning,
    WebUndoActionLoosenKerning,
    WebUndoActionUseStandardKerning,
    WebUndoActionTurnOffLigatures,
    WebUndoActionUseStandardLigatures,
    WebUndoActionUseAllLigatures,
    WebUndoActionRaiseBaseline,
    WebUndoActionLowerBaseline,
    WebUndoActionSetTraditionalCharacterShape,
    WebUndoActionSetFont,
    WebUndoActionChangeAttributes,
    WebUndoActionAlignLeft,
    WebUndoActionAlignRight,
    WebUndoActionCenter,
    WebUndoActionJustify,
    WebUndoActionSetWritingDirection,
    WebUndoActionSubscript,
    WebUndoActionSuperscript,
    WebUndoActionUnderline,
    WebUndoActionOutline,
    WebUndoActionUnscript,
    WebUndoActionDrag,
    WebUndoActionCut,
    WebUndoActionPaste,
    WebUndoActionPasteFont,
    WebUndoActionPasteRuler,
    WebUndoActionTyping,
    WebUndoActionCreateLink,
    WebUndoActionUnlink,
    WebUndoActionFormatBlock,
    WebUndoActionInsertList,
    WebUndoActionIndent,
    WebUndoActionOutdent
} WebUndoAction;

typedef enum {
    ObjectElementNone,
    ObjectElementImage,
    ObjectElementFrame,
    ObjectElementPlugin
} ObjectElementType;

@protocol WebCoreOpenPanelResultListener <NSObject>
- (void)chooseFilename:(NSString *)fileName;
- (void)cancel;
@end

// WebCoreFrameBridge objects are used by WebCore to abstract away operations that need
// to be implemented by library clients, for example WebKit. The objects are also
// used in the opposite direction, for simple access to WebCore functions without dealing
// directly with the KHTML C++ classes.

// A WebCoreFrameBridge creates and holds a reference to a Frame.

// The WebCoreFrameBridge interface contains methods for use by the non-WebCore side of the bridge.

@interface WebCoreFrameBridge : NSObject
{
    WebCoreMacFrame *m_frame;
    BOOL _shouldCreateRenderers;
    BOOL _closed;
}

+ (WebCoreFrameBridge *)bridgeForDOMDocument:(DOMDocument *)document;

- (id)initMainFrameWithPage:(WebCorePageBridge *)page;
- (id)initSubframeWithOwnerElement:(WebCoreElement *)ownerElement;

- (void)close;

+ (NSArray *)supportedNonImageMIMETypes;
+ (NSArray *)supportedImageMIMETypes;
+ (NSArray *)supportedImageResourceMIMETypes; // includes types like PDF

- (void)initializeSettings:(WebCoreSettings *)settings;

- (void)setName:(NSString *)name;
- (NSString *)name;

- (WebCorePageBridge *)page;

- (WebCoreFrameBridge *)parent;

- (WebCoreFrameBridge *)firstChild;
- (WebCoreFrameBridge *)lastChild;
- (WebCoreFrameBridge *)previousSibling;
- (WebCoreFrameBridge *)nextSibling;

- (void)appendChild:(WebCoreFrameBridge *)child;
- (void)removeChild:(WebCoreFrameBridge *)child;

- (unsigned)childCount;
- (BOOL)isDescendantOfFrame:(WebCoreFrameBridge *)ancestor;
- (WebCoreFrameBridge *)traverseNextFrameStayWithin:(WebCoreFrameBridge *)stayWithin;

- (WebCoreFrameBridge *)nextFrameWithWrap:(BOOL)wrap;
- (WebCoreFrameBridge *)previousFrameWithWrap:(BOOL)wrap;

- (WebCoreFrameBridge *)childFrameNamed:(NSString *)name;
- (WebCoreFrameBridge *)findFrameNamed:(NSString *)name;

- (void)provisionalLoadStarted;

- (void)openURL:(NSURL *)URL reload:(BOOL)reload
    contentType:(NSString *)contentType refresh:(NSString *)refresh lastModified:(NSDate *)lastModified
    pageCache:(NSDictionary *)pageCache;
- (void)setEncoding:(NSString *)encoding userChosen:(BOOL)userChosen;
- (void)addData:(NSData *)data;
- (void)closeURL;
- (void)stopLoading;

- (void)didNotOpenURL:(NSURL *)URL pageCache:(NSDictionary *)pageCache;

- (BOOL)canLoadURL:(NSURL *)URL fromReferrer:(NSString *)referrer hideReferrer:(BOOL *)hideReferrer;
- (BOOL)canTargetLoadInFrame:(WebCoreFrameBridge *)targetFrame;

- (void)saveDocumentState;
- (void)restoreDocumentState;

- (BOOL)canCachePage;
- (BOOL)saveDocumentToPageCache;

- (void)end;
- (void)stop;

- (void)clearFrame;
- (void)handleFallbackContent;

- (NSURL *)URL;
- (NSURL *)baseURL;
- (NSString *)referrer;
- (WebCoreFrameBridge *)opener;
- (void)setOpener:(WebCoreFrameBridge *)bridge;

- (void)installInFrame:(NSView *)view;
- (void)removeFromFrame;

- (void)scrollToAnchorWithURL:(NSURL *)URL;

- (BOOL)scrollOverflowInDirection:(WebScrollDirection)direction granularity:(WebScrollGranularity)granularity;

- (void)createFrameViewWithNSView:(NSView *)view marginWidth:(int)mw marginHeight:(int)mh;

- (BOOL)isFrameSet;

- (void)reapplyStylesForDeviceType:(WebCoreDeviceType)deviceType;
- (void)forceLayoutAdjustingViewSize:(BOOL)adjustSizeFlag;
- (void)forceLayoutWithMinimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustingViewSize:(BOOL)adjustSizeFlag;
- (void)sendResizeEvent;
- (void)sendScrollEvent;
- (BOOL)needsLayout;
- (void)setNeedsLayout;
- (void)drawRect:(NSRect)rect;
- (void)adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit;
- (NSArray*)computePageRectsWithPrintWidthScaleFactor:(float)printWidthScaleFactor printHeight:(float)printHeight;

- (void)setActivationEventNumber:(int)num;
- (void)mouseDown:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;

// these return YES if event is eaten by WebCore
- (BOOL)sendScrollWheelEvent:(NSEvent *)event;
- (BOOL)sendContextMenuEvent:(NSEvent *)event;

- (NSView *)nextKeyView;
- (NSView *)previousKeyView;

- (NSView *)nextKeyViewInsideWebFrameViews;
- (NSView *)previousKeyViewInsideWebFrameViews;

- (NSObject *)copyRenderTree:(id <WebCoreRenderTreeCopier>)copier;
- (NSString *)renderTreeAsExternalRepresentation;

- (void)getInnerNonSharedNode:(DOMNode **)innerNonSharedNode innerNode:(DOMNode **)innerNode URLElement:(DOMElement **)URLElement atPoint:(NSPoint)point allowShadowContent:(BOOL)allow;
- (BOOL)isPointInsideSelection:(NSPoint)point;

- (NSURL *)URLWithAttributeString:(NSString *)string;

- (DOMElement *)elementWithName:(NSString *)name inForm:(DOMElement *)form;
- (DOMElement *)elementForView:(NSView *)view;
- (BOOL)elementDoesAutoComplete:(DOMElement *)element;
- (BOOL)elementIsPassword:(DOMElement *)element;
- (DOMElement *)formForElement:(DOMElement *)element;
- (DOMElement *)currentForm;
- (NSArray *)controlsInForm:(DOMElement *)form;
- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element;
- (NSString *)matchLabels:(NSArray *)labels againstElement:(DOMElement *)element;

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag;
- (unsigned)markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag;
- (BOOL)markedTextMatchesAreHighlighted;
- (void)setMarkedTextMatchesAreHighlighted:(BOOL)doHighlight;
- (void)unmarkAllTextMatches;
- (NSArray *)rectsForTextMatches;

- (NSString *)advanceToNextMisspelling;
- (NSString *)advanceToNextMisspellingStartingJustBeforeSelection;
- (void)unmarkAllMisspellings;

- (void)setTextSizeMultiplier:(float)multiplier;

- (CFStringEncoding)textEncoding;

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string;
- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string forceUserGesture:(BOOL)forceUserGesture;
- (NSAppleEventDescriptor *)aeDescByEvaluatingJavaScriptFromString:(NSString *)string;

- (DOMDocument *)DOMDocument;
- (DOMHTMLElement *)frameElement;

- (BOOL)isSelectionEditable;
- (BOOL)isSelectionRichlyEditable;
- (WebSelectionState)selectionState;

- (NSAttributedString *)selectedAttributedString;
- (NSString *)selectedString;

- (void)setSelectionFromNone;
- (void)setIsActive:(BOOL)flag;

- (void)setWindowHasFocus:(BOOL)flag;

- (NSString *)stringForRange:(DOMRange *)range;

- (NSString *)markupStringFromNode:(DOMNode *)node nodes:(NSArray **)nodes;
- (NSString *)markupStringFromRange:(DOMRange *)range nodes:(NSArray **)nodes;

- (void)selectAll;
- (void)deselectAll;
- (void)deselectText;

- (NSRect)selectionRect;
- (NSRect)visibleSelectionRect;
- (void)centerSelectionInVisibleArea;
- (NSImage *)selectionImageForcingWhiteText:(BOOL)forceWhiteText;
- (NSRect)caretRectAtNode:(DOMNode *)node offset:(int)offset affinity:(NSSelectionAffinity)affinity;
- (NSRect)firstRectForDOMRange:(DOMRange *)range;

- (void)setSelectedDOMRange:(DOMRange *)range affinity:(NSSelectionAffinity)selectionAffinity closeTyping:(BOOL)closeTyping;
- (DOMRange *)selectedDOMRange;
- (NSSelectionAffinity)selectionAffinity;

// Emacs-style-editing "mark"
- (void)setMarkDOMRange:(DOMRange *)range;
- (DOMRange *)markDOMRange;

// international text input "marked text"
- (void)setMarkedTextDOMRange:(DOMRange *)range customAttributes:(NSArray *)attributes ranges:(NSArray *)ranges;
- (DOMRange *)markedTextDOMRange;
- (void)replaceMarkedTextWithText:(NSString *)text;

- (NSAttributedString *)attributedStringFrom:(DOMNode *)startNode startOffset:(int)startOffset to:(DOMNode *)endNode endOffset:(int)endOffset;

- (NSFont *)fontForSelection:(BOOL *)hasMultipleFonts;
- (NSDictionary *)fontAttributesForSelectionStart;
- (NSWritingDirection)baseWritingDirectionForSelectionStart;

+ (NSString *)stringWithData:(NSData *)data textEncoding:(CFStringEncoding)textEncoding;
+ (NSString *)stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName;

- (BOOL)interceptKeyEvent:(NSEvent *)event toView:(NSView *)view;

- (void)setShouldCreateRenderers:(BOOL)f;
- (BOOL)shouldCreateRenderers;

- (int)numPendingOrLoadingRequests;
- (BOOL)doneProcessingData;
- (BOOL)shouldClose;

- (void)setDrawsBackground:(BOOL)drawsBackround;

- (NSColor *)bodyBackgroundColor;
- (NSColor *)selectionColor;

- (void)adjustViewSize;

- (id)accessibilityTree;

- (void)undoEditing:(id)arg;
- (void)redoEditing:(id)arg;

- (DOMRange *)rangeByExpandingSelectionWithGranularity:(WebBridgeSelectionGranularity)granularity;
- (DOMRange *)rangeOfCharactersAroundCaret;
- (DOMRange *)rangeByAlteringCurrentSelection:(WebSelectionAlteration)alteration direction:(WebBridgeSelectionDirection)direction granularity:(WebBridgeSelectionGranularity)granularity;
- (void)alterCurrentSelection:(WebSelectionAlteration)alteration direction:(WebBridgeSelectionDirection)direction granularity:(WebBridgeSelectionGranularity)granularity;
- (DOMRange *)rangeByAlteringCurrentSelection:(WebSelectionAlteration)alteration verticalDistance:(float)distance;
- (void)alterCurrentSelection:(WebSelectionAlteration)alteration verticalDistance:(float)distance;
- (WebBridgeSelectionGranularity)selectionGranularity;
- (DOMRange *)smartDeleteRangeForProposedRange:(DOMRange *)proposedCharRange;
- (void)smartInsertForString:(NSString *)pasteString replacingRange:(DOMRange *)charRangeToReplace beforeString:(NSString **)beforeString afterString:(NSString **)afterString;
- (BOOL)canDeleteRange:(DOMRange *)range;
- (void)selectNSRange:(NSRange)range;
- (NSRange)selectedNSRange;
- (NSRange)markedTextNSRange;
- (DOMRange *)convertNSRangeToDOMRange:(NSRange)range;
- (NSRange)convertDOMRangeToNSRange:(DOMRange *)range;

- (DOMDocumentFragment *)documentFragmentWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString;
- (DOMDocumentFragment *)documentFragmentWithText:(NSString *)text inContext:(DOMRange *)context;
- (DOMDocumentFragment *)documentFragmentWithNodesAsParagraphs:(NSArray *)nodes;

- (void)replaceSelectionWithFragment:(DOMDocumentFragment *)fragment selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle;
- (void)replaceSelectionWithNode:(DOMNode *)node selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle;
- (void)replaceSelectionWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace;
- (void)replaceSelectionWithText:(NSString *)text selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace;

- (bool)canIncreaseSelectionListLevel;
- (bool)canDecreaseSelectionListLevel;
- (DOMNode *)increaseSelectionListLevel;
- (DOMNode *)increaseSelectionListLevelOrdered;
- (DOMNode *)increaseSelectionListLevelUnordered;
- (void)decreaseSelectionListLevel;

- (void)insertLineBreak;
- (void)insertParagraphSeparator;
- (void)insertParagraphSeparatorInQuotedContent;
- (void)insertText:(NSString *)text selectInsertedText:(BOOL)selectInsertedText;

- (void)setSelectionToDragCaret;
- (void)moveSelectionToDragCaret:(DOMDocumentFragment *)selectionFragment smartMove:(BOOL)smartMove;
- (void)moveDragCaretToPoint:(NSPoint)point;
- (void)removeDragCaret;
- (DOMRange *)dragCaretDOMRange;
- (BOOL)isDragCaretRichlyEditable;
- (DOMRange *)editableDOMRangeForPoint:(NSPoint)point;
- (DOMRange *)characterRangeAtPoint:(NSPoint)point;

- (void)deleteSelectionWithSmartDelete:(BOOL)smartDelete;
- (void)deleteKeyPressedWithSmartDelete:(BOOL)smartDelete granularity:(WebBridgeSelectionGranularity)granularity;
- (void)forwardDeleteKeyPressedWithSmartDelete:(BOOL)smartDelete granularity:(WebBridgeSelectionGranularity)granularity;

- (DOMCSSStyleDeclaration *)typingStyle;
- (void)setTypingStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(WebUndoAction)undoAction;
- (void)applyStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(WebUndoAction)undoAction;
- (void)applyParagraphStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(WebUndoAction)undoAction;
- (BOOL)selectionStartHasStyle:(DOMCSSStyleDeclaration *)style;
- (NSCellStateValue)selectionHasStyle:(DOMCSSStyleDeclaration *)style;
- (void)applyEditingStyleToBodyElement;
- (void)removeEditingStyleFromBodyElement;
- (void)applyEditingStyleToElement:(DOMElement *)element;
- (void)removeEditingStyleFromElement:(DOMElement *)element;

- (void)ensureSelectionVisible;

- (WebScriptObject *)windowScriptObject;
- (NPObject *)windowScriptNPObject;

- (BOOL)eventMayStartDrag:(NSEvent *)event;
- (NSDragOperation)dragOperationForDraggingInfo:(id <NSDraggingInfo>)info;
- (void)dragExitedWithDraggingInfo:(id <NSDraggingInfo>)info;
- (BOOL)concludeDragForDraggingInfo:(id <NSDraggingInfo>)info;
- (void)dragSourceMovedTo:(NSPoint)windowLoc;
- (void)dragSourceEndedAt:(NSPoint)windowLoc operation:(NSDragOperation)operation;

- (BOOL)mayDHTMLCut;
- (BOOL)mayDHTMLCopy;
- (BOOL)mayDHTMLPaste;
- (BOOL)tryDHTMLCut;
- (BOOL)tryDHTMLCopy;
- (BOOL)tryDHTMLPaste;

- (NSMutableDictionary *)dashboardRegions;

- (void)clear;

- (BOOL)isCharacterSmartReplaceExempt:(unichar)c isPreviousCharacter:(BOOL)isPreviousCharacter;

- (BOOL)getData:(NSData **)data andResponse:(NSURLResponse **)response forURL:(NSURL *)URL;
- (void)getAllResourceDatas:(NSArray **)datas andResponses:(NSArray **)responses;

- (BOOL)canProvideDocumentSource;
- (BOOL)canSaveAsWebArchive;
- (BOOL)containsPlugins;

- (void)setInViewSourceMode:(BOOL)flag;
- (BOOL)inViewSourceMode;

- (void)setProhibitsScrolling:(BOOL)prohibits;

@end

// The WebCoreFrameBridge protocol contains methods for use by the WebCore side of the bridge.

// In NSArray objects for post data, NSData objects represent literal data, and NSString objects represent encoded files.
// The encoding is the standard form encoding for uploading files.

@protocol WebCoreFrameBridge

- (void)frameDetached;
- (NSView *)documentView;

- (void)loadURL:(NSURL *)URL referrer:(NSString *)referrer reload:(BOOL)reload userGesture:(BOOL)forUser target:(NSString *)target triggeringEvent:(NSEvent *)event form:(DOMElement *)form formValues:(NSDictionary *)values;
- (void)postWithURL:(NSURL *)URL referrer:(NSString *)referrer target:(NSString *)target data:(NSArray *)data contentType:(NSString *)contentType triggeringEvent:(NSEvent *)event form:(DOMElement *)form formValues:(NSDictionary *)values;

- (WebCorePageBridge *)createWindowWithURL:(NSURL *)URL;
- (void)showWindow;

- (BOOL)canRunModal;
- (BOOL)canRunModalNow;
- (WebCorePageBridge *)createModalDialogWithURL:(NSURL *)URL;
- (void)runModal;

- (NSString *)userAgentForURL:(NSURL *)URL;

- (void)setTitle:(NSString *)title;
- (void)setStatusText:(NSString *)status;

- (void)setIconURL:(NSURL *)URL;
- (void)setIconURL:(NSURL *)URL withType:(NSString *)string;

- (WebCoreFrameBridge *)createChildFrameNamed:(NSString *)frameName withURL:(NSURL *)URL
    referrer:(NSString *)referrer
    ownerElement:(WebCoreElement *)ownerElement
    allowsScrolling:(BOOL)allowsScrolling marginWidth:(int)width marginHeight:(int)height;

- (BOOL)areToolbarsVisible;
- (void)setToolbarsVisible:(BOOL)visible;
- (BOOL)isStatusbarVisible;
- (void)setStatusbarVisible:(BOOL)visible;
- (BOOL)areScrollbarsVisible;
- (void)setScrollbarsVisible:(BOOL)visible;
- (NSWindow *)window;

- (void)setWindowIsResizable:(BOOL)resizable;
- (BOOL)windowIsResizable;

- (NSResponder *)firstResponder;
- (void)makeFirstResponder:(NSResponder *)responder;
- (void)willMakeFirstResponderForNodeFocus;

- (BOOL)textViewWasFirstResponderAtMouseDownTime:(NSTextView *)textView;

- (void)closeWindowSoon;

- (void)runJavaScriptAlertPanelWithMessage:(NSString *)message;
- (BOOL)runJavaScriptConfirmPanelWithMessage:(NSString *)message;
- (BOOL)runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt defaultText:(NSString *)defaultText returningText:(NSString **)result;
- (BOOL)shouldInterruptJavaScript;
- (BOOL)canRunBeforeUnloadConfirmPanel;
- (BOOL)runBeforeUnloadConfirmPanelWithMessage:(NSString *)message;
- (void)addMessageToConsole:(NSDictionary *)message;
- (void)runOpenPanelForFileButtonWithResultListener:(id <WebCoreOpenPanelResultListener>)resultListener;

- (id <WebCoreResourceHandle>)startLoadingResource:(id <WebCoreResourceLoader>)loader withMethod:(NSString *)method URL:(NSURL *)URL customHeaders:(NSDictionary *)customHeaders;
- (id <WebCoreResourceHandle>)startLoadingResource:(id <WebCoreResourceLoader>)loader withMethod:(NSString *)method URL:(NSURL *)URL customHeaders:(NSDictionary *)customHeaders postData:(NSArray *)data;
- (void)objectLoadedFromCacheWithURL:(NSURL *)URL response:(NSURLResponse *)response data:(NSData *)data;

- (NSData *)syncLoadResourceWithMethod:(NSString *)method URL:(NSURL *)URL customHeaders:(NSDictionary *)requestHeaders postData:(NSArray *)postData finalURL:(NSURL **)finalNSURL responseHeaders:(NSDictionary **)responseHeaderDict statusCode:(int *)statusCode;

- (BOOL)isReloading;
- (time_t)expiresTimeForResponse:(NSURLResponse *)response;

- (void)reportClientRedirectToURL:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date lockHistory:(BOOL)lockHistory isJavaScriptFormAction:(BOOL)isJavaScriptFormAction;
- (void)reportClientRedirectCancelled:(BOOL)cancelWithLoadInProgress;

- (void)activateWindow;
- (void)deactivateWindow;

- (void)formControlIsBecomingFirstResponder:(NSView *)formControl;
- (void)formControlIsResigningFirstResponder:(NSView *)formControl;

- (NSView *)nextKeyViewOutsideWebFrameViews;
- (NSView *)nextValidKeyViewOutsideWebFrameViews;
- (NSView *)previousKeyViewOutsideWebFrameViews;

- (BOOL)defersLoading;
- (void)setDefersLoading:(BOOL)loading;
- (void)saveDocumentState:(NSArray *)documentState;
- (NSArray *)documentState;

- (void)setNeedsReapplyStyles;

- (void)tokenizerProcessedData;

- (NSString *)incomingReferrer;

- (NSView *)viewForPluginWithURL:(NSURL *)URL
                  attributeNames:(NSArray *)attributeNames
                 attributeValues:(NSArray *)attributeValues
                        MIMEType:(NSString *)MIMEType
                      DOMElement:(DOMElement *)element
                    loadManually:(BOOL)loadManually;
- (NSView *)viewForJavaAppletWithFrame:(NSRect)frame
                        attributeNames:(NSArray *)attributeNames
                       attributeValues:(NSArray *)attributeValues
                               baseURL:(NSURL *)baseURL
                            DOMElement:(DOMElement *)element;
- (void)redirectDataToPlugin:(NSView *)pluginView;

- (BOOL)saveDocumentToPageCache:(id)documentInfo;

- (int)getObjectCacheSize;

- (ObjectElementType)determineObjectFromMIMEType:(NSString*)MIMEType URL:(NSURL*)URL;

- (void)loadEmptyDocumentSynchronously;

- (NSString *)MIMETypeForPath:(NSString *)path;

- (void)allowDHTMLDrag:(BOOL *)flagDHTML UADrag:(BOOL *)flagUA;
- (BOOL)startDraggingImage:(NSImage *)dragImage at:(NSPoint)dragLoc operation:(NSDragOperation)op event:(NSEvent *)event sourceIsDHTML:(BOOL)flag DHTMLWroteData:(BOOL)dhtmlWroteData;
- (void)handleAutoscrollForMouseDragged:(NSEvent *)event;
- (BOOL)mayStartDragAtEventLocation:(NSPoint)location;

- (BOOL)selectWordBeforeMenuEvent;

- (int)historyLength;
- (void)goBackOrForward:(int)distance;
- (BOOL)canGoBackOrForward:(int)distance;
- (NSURL *)historyURL:(int)distance;

- (void)textFieldDidBeginEditing:(DOMHTMLInputElement *)element;
- (void)textFieldDidEndEditing:(DOMHTMLInputElement *)element;
- (void)textDidChangeInTextField:(DOMHTMLInputElement *)element;
- (void)textDidChangeInTextArea:(DOMHTMLTextAreaElement *)element;

- (BOOL)textField:(DOMHTMLInputElement *)element doCommandBySelector:(SEL)commandSelector;
- (BOOL)textField:(DOMHTMLInputElement *)element shouldHandleEvent:(NSEvent *)event;

- (void)setHasBorder:(BOOL)hasBorder;

- (NSFileWrapper *)fileWrapperForURL:(NSURL *)URL;

- (void)print;

- (jobject)getAppletInView:(NSView *)view;

// Deprecated, use getAppletInView: instead.
- (jobject)pollForAppletInView:(NSView *)view;

- (NSUndoManager *)undoManager;
- (NSString *)nameForUndoAction:(WebUndoAction)undoAction;
- (void)issueCutCommand;
- (void)issueCopyCommand;
- (void)issuePasteCommand;
- (void)issuePasteAndMatchStyleCommand;
- (void)issueTransposeCommand;
- (void)respondToChangedSelection;
- (void)respondToChangedContents;
- (void)setIsSelected:(BOOL)isSelected forView:(NSView *)view;
- (BOOL)isEditable;
- (BOOL)shouldChangeSelectedDOMRange:(DOMRange *)currentRange toDOMRange:(DOMRange *)proposedRange affinity:(NSSelectionAffinity)selectionAffinity stillSelecting:(BOOL)flag;
- (BOOL)shouldDeleteSelectedDOMRange:(DOMRange *)currentRange;
- (BOOL)shouldBeginEditing:(DOMRange *)range;
- (BOOL)shouldEndEditing:(DOMRange *)range;
- (void)didBeginEditing;
- (void)didEndEditing;
- (BOOL)canPaste;

- (NSString *)overrideMediaType;

- (void)windowObjectCleared;

- (int)spellCheckerDocumentTag;
- (BOOL)isContinuousSpellCheckingEnabled;

- (void)didFirstLayout;

- (void)dashboardRegionsChanged:(NSMutableDictionary *)regions;

- (NSRect)customHighlightRect:(NSString*)type forLine:(NSRect)lineRect;
- (void)paintCustomHighlight:(NSString*)type forBox:(NSRect)boxRect onLine:(NSRect)lineRect behindText:(BOOL)text entireLine:(BOOL)line;

- (void)handledOnloadEvents;

- (WebCoreKeyboardUIMode)keyboardUIMode;

- (NSURLResponse*)mainResourceURLResponse;
- (NSString*)imageTitleForFilename:(NSString*)filename size:(NSSize)size;

@end

// This interface definition allows those who hold a WebCoreFrameBridge * to call all the methods
// in the WebCoreFrameBridge protocol without requiring the base implementation to supply the methods.
// This idiom is appropriate because WebCoreFrameBridge is an abstract class.

@interface WebCoreFrameBridge (SubclassResponsibility) <WebCoreFrameBridge>
@end

// One method for internal use within WebCore itself.
// Could move this to another header, but would be a pity to create an entire header just for that.

@interface WebCoreFrameBridge (WebCoreInternalUse)
- (WebCoreMacFrame*)impl;
@end

// Protocols that make up part of the interaces above.

@protocol WebCoreRenderTreeCopier <NSObject>
- (NSObject *)nodeWithName:(NSString *)name position:(NSPoint)p rect:(NSRect)rect view:(NSView *)view children:(NSArray *)children;
@end

