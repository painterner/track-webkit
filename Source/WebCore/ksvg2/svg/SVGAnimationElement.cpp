/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>

    This file is part of the KDE project

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
*/

#include "config.h"
#ifdef SVG_SUPPORT
#include "SVGAnimationElement.h"

#include "CSSPropertyNames.h"
#include "Document.h"
#include "TimeScheduler.h"
#include "SVGSVGElement.h"
#include "SVGURIReference.h"
#include "XLinkNames.h"
#include "ksvgcssproperties.h"
#include <float.h>
#include <math.h>
#include <wtf/Vector.h>
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

SVGAnimationElement::SVGAnimationElement(const QualifiedName& tagName, Document* doc)
    : SVGElement(tagName, doc)
    , SVGTests()
    , SVGExternalResourcesRequired()
    , m_targetElement(0)
    , m_connectedToTimer(false)
    , m_currentTime(0.0)
    , m_simpleDuration(0.0)
    , m_fill(FILL_REMOVE)
    , m_restart(RESTART_ALWAYS)
    , m_calcMode(CALCMODE_LINEAR)
    , m_additive(ADDITIVE_REPLACE)
    , m_accumulate(ACCUMULATE_NONE)
    , m_attributeType(ATTRIBUTETYPE_AUTO)
    , m_max(0.0)
    , m_min(0.0)
    , m_end(0.0)
    , m_begin(0.0)
    , m_repetitions(0)
    , m_repeatCount(0)
{

}

SVGAnimationElement::~SVGAnimationElement()
{
}

bool SVGAnimationElement::hasValidTarget() const
{
    return targetElement();
}

SVGElement* SVGAnimationElement::targetElement() const
{
    if (!m_targetElement) {
        if (!m_href.isEmpty()) {
            Element* element = ownerDocument()->getElementById(SVGURIReference::getTarget(m_href));
            m_targetElement = svg_dynamic_cast(element);
        } else if (parentNode()) {
            Node* target = parentNode();
            while (target) {
                if (target->nodeType() != ELEMENT_NODE)
                    target = target->parentNode();
                else
                    break;
            }
            m_targetElement = svg_dynamic_cast(target);
        }
    }
                        
    return m_targetElement;
}

double SVGAnimationElement::getEndTime() const
{
    return m_end;
}

double SVGAnimationElement::getStartTime() const
{
    return m_begin;
}

double SVGAnimationElement::getCurrentTime() const
{
    return m_currentTime;
}

double SVGAnimationElement::getSimpleDuration(ExceptionCode&) const
{
    return m_simpleDuration;
}

void SVGAnimationElement::parseMappedAttribute(MappedAttribute* attr)
{
    const String& value = attr->value();
    if (attr->name().matches(XLinkNames::hrefAttr))
        m_href = value;
    else if (attr->name() == SVGNames::attributeNameAttr)
        m_attributeName = value;
    else if (attr->name() == SVGNames::attributeTypeAttr) {
        if (value == "CSS")
            m_attributeType = ATTRIBUTETYPE_CSS;
        else if (value == "XML")
            m_attributeType = ATTRIBUTETYPE_XML;
        else if (value == "auto")
            m_attributeType = ATTRIBUTETYPE_AUTO;
    } else if (attr->name() == SVGNames::beginAttr || attr->name() == SVGNames::endAttr) {
        RefPtr<SVGStringList> valueList = new SVGStringList();
        valueList->parse(value, ';');

        ExceptionCode ec = 0;
        for (unsigned int i = 0; i < valueList->numberOfItems(); i++) {
            String current = valueList->getItem(i, ec);

            if (current.startsWith("accessKey")) {
                // Register keyDownEventListener for the character
                String character = current.substring(current.length() - 2, 1);
                // FIXME: Not implemented! Supposed to register accessKey character
            } else if (current.startsWith("wallclock")) {
                int firstBrace = current.find('(');
                int secondBrace = current.find(')');

                String wallclockValue = current.substring(firstBrace + 1, secondBrace - firstBrace - 2);
                // FIXME: Not implemented! Supposed to use wallClock value
            } else if (current.contains('.')) {
                int dotPosition = current.find('.');

                String element = current.substring(0, dotPosition);
                String clockValue;
                if (current.contains("begin"))
                    clockValue = current.substring(dotPosition + 6);
                else if (current.contains("end"))
                    clockValue = current.substring(dotPosition + 4);
                else if (current.contains("repeat"))
                    clockValue = current.substring(dotPosition + 7);
                else { // DOM2 Event Reference
                    int plusMinusPosition = -1;

                    if (current.contains('+'))
                        plusMinusPosition = current.find('+');
                    else if (current.contains('-'))
                        plusMinusPosition = current.find('-');

                    String event = current.substring(dotPosition + 1, plusMinusPosition - dotPosition - 1);
                    clockValue = current.substring(dotPosition + event.length() + 1);
                    // FIXME: supposed to use DOM Event
                }
            } else {
                if (attr->name() == SVGNames::beginAttr) {
                    m_begin = parseClockValue(current);
                    if (!isIndefinite(m_begin))
                        m_begin *= 1000.0;
                    // FIXME: supposed to set begin time
                } else {
                    m_end = parseClockValue(current);
                    if (!isIndefinite(m_end))
                        m_end *= 1000.0;
                    // FIXME: supposed to set end time
                }
            }
        }
    } else if (attr->name() == SVGNames::durAttr) {
        m_simpleDuration = parseClockValue(value);
        if (!isIndefinite(m_simpleDuration))
            m_simpleDuration *= 1000.0;
    } else if (attr->name() == SVGNames::minAttr) {
        m_min = parseClockValue(value);
        if (!isIndefinite(m_min))
            m_min *= 1000.0;
    } else if (attr->name() == SVGNames::maxAttr) {
        m_max = parseClockValue(value);
        if (!isIndefinite(m_max))
            m_max *= 1000.0;
    } else if (attr->name() == SVGNames::restartAttr) {
        if (value == "whenNotActive")
            m_restart = RESTART_WHENNOTACTIVE;
        else if (value == "never")
            m_restart = RESTART_NEVER;
        else if (value == "always")
            m_restart = RESTART_ALWAYS;
    } else if (attr->name() == SVGNames::repeatCountAttr) {
        if (value == "indefinite")
            m_repeatCount = DBL_MAX;
        else
            m_repeatCount = value.toDouble();
    } else if (attr->name() == SVGNames::repeatDurAttr)
        m_repeatDur = value;
    else if (attr->name() == SVGNames::fillAttr) {
        if (value == "freeze")
            m_fill = FILL_FREEZE;
        else if (value == "remove")
            m_fill = FILL_REMOVE;
    } else if (attr->name() == SVGNames::calcModeAttr) {
        if (value == "discrete")
            m_calcMode = CALCMODE_DISCRETE;
        else if (value == "linear")
            m_calcMode = CALCMODE_LINEAR;
        else if (value == "spline")
            m_calcMode = CALCMODE_SPLINE;
        else if (value == "paced")
            m_calcMode = CALCMODE_PACED;
    } else if (attr->name() == SVGNames::valuesAttr) {
        m_values = new SVGStringList();
        m_values->parse(value, ';');
    } else if (attr->name() == SVGNames::keyTimesAttr) {
        m_keyTimes = new SVGStringList();
        m_keyTimes->parse(value, ';');
    } else if (attr->name() == SVGNames::keySplinesAttr) {
        m_keySplines = new SVGStringList();
        m_keySplines->parse(value, ';');
    } else if (attr->name() == SVGNames::fromAttr)
        m_from = value;
    else if (attr->name() == SVGNames::toAttr)
        m_to = value;
    else if (attr->name() == SVGNames::byAttr)
        m_by = value;
    else if (attr->name() == SVGNames::additiveAttr) {
        if (value == "sum")
            m_additive = ADDITIVE_SUM;
        else if (value == "replace")
            m_additive = ADDITIVE_REPLACE;
    } else if (attr->name() == SVGNames::accumulateAttr) {
        if (value == "sum")
            m_accumulate = ACCUMULATE_SUM;
        else if (value == "none")
            m_accumulate = ACCUMULATE_NONE;
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGElement::parseMappedAttribute(attr);
    }
}

double SVGAnimationElement::parseClockValue(const String& data) const
{
    DeprecatedString parse = data.deprecatedString().stripWhiteSpace();
    
    if (parse == "indefinite") // Saves some time...
        return DBL_MAX;

    double result;

    int doublePointOne = parse.find(':');
    int doublePointTwo = parse.find(':', doublePointOne + 1);

    if (doublePointOne != -1 && doublePointTwo != -1) { // Spec: "Full clock values"
        unsigned int hours = parse.mid(0, 2).toUInt();
        unsigned int minutes = parse.mid(3, 2).toUInt();
        unsigned int seconds = parse.mid(6, 2).toUInt();
        unsigned int milliseconds = 0;

        result = (3600 * hours) + (60 * minutes) + seconds;

        if (parse.find('.') != -1) {
            DeprecatedString temp = parse.mid(9, 2);
            milliseconds = temp.toUInt();
            result += (milliseconds * (1 / pow(10.0, int(temp.length()))));
        }
    } else if (doublePointOne != -1 && doublePointTwo == -1) { // Spec: "Partial clock values"
        unsigned int minutes = parse.mid(0, 2).toUInt();
        unsigned int seconds = parse.mid(3, 2).toUInt();
        unsigned int milliseconds = 0;

        result = (60 * minutes) + seconds;

        if (parse.find('.') != -1) {
            DeprecatedString temp = parse.mid(6, 2);
            milliseconds = temp.toUInt();
            result += (milliseconds * (1 / pow(10.0, int(temp.length()))));
        }
    } else { // Spec: "Timecount values"
        int dotPosition = parse.find('.');

        if (parse.endsWith("h")) {
            if (dotPosition == -1)
                result = parse.mid(0, parse.length() - 1).toUInt() * 3600;
            else {
                result = parse.mid(0, dotPosition).toUInt() * 3600;
                DeprecatedString temp = parse.mid(dotPosition + 1, parse.length() - dotPosition - 2);
                result += (3600.0 * temp.toUInt()) * (1 / pow(10.0, int(temp.length())));
            }
        } else if (parse.endsWith("min")) {
            if (dotPosition == -1)
                result = parse.mid(0, parse.length() - 3).toUInt() * 60;
            else {
                result = parse.mid(0, dotPosition).toUInt() * 60;
                DeprecatedString temp = parse.mid(dotPosition + 1, parse.length() - dotPosition - 4);
                result += (60.0 * temp.toUInt()) * (1 / pow(10.0, int(temp.length())));
            }
        } else if (parse.endsWith("ms")) {
            if (dotPosition == -1)
                result = parse.mid(0, parse.length() - 2).toUInt() / 1000.0;
            else {
                result = parse.mid(0, dotPosition).toUInt() / 1000.0;
                DeprecatedString temp = parse.mid(dotPosition + 1, parse.length() - dotPosition - 3);
                result += (temp.toUInt() / 1000.0) * (1 / pow(10.0, int(temp.length())));
            }
        } else if (parse.endsWith("s")) {
            if (dotPosition == -1)
                result = parse.mid(0, parse.length() - 1).toUInt();
            else {
                result = parse.mid(0, dotPosition).toUInt();
                DeprecatedString temp = parse.mid(dotPosition + 1, parse.length() - dotPosition - 2);
                result += temp.toUInt() * (1 / pow(10.0, int(temp.length())));
            }
        } else
            result = parse.toDouble();
    }

    return result;
}

void SVGAnimationElement::closeRenderer()
{
    ownerSVGElement()->timeScheduler()->addTimer(this, lround(getStartTime()));
    SVGElement::closeRenderer();
}

String SVGAnimationElement::targetAttribute() const
{
    if (!targetElement())
        return String();
    
    SVGElement* target = targetElement();
    SVGStyledElement* styled = 0;
    if (target && target->isStyled())
        styled = static_cast<SVGStyledElement*>(target);
    
    String ret;

    EAttributeType attributeType = static_cast<EAttributeType>(m_attributeType);
    if (attributeType == ATTRIBUTETYPE_AUTO) {
        attributeType = ATTRIBUTETYPE_XML;

        // Spec: The implementation should match the attributeName to an attribute
        // for the target element. The implementation must first search through the
        // list of CSS properties for a matching property name, and if none is found,
        // search the default XML namespace for the element.
        if (styled && styled->style()) {
            if (styled->style()->getPropertyCSSValue(m_attributeName))
                attributeType = ATTRIBUTETYPE_CSS;
        }
    }
    
    if (attributeType == ATTRIBUTETYPE_CSS) {
        if (styled && styled->style())
            ret = styled->style()->getPropertyValue(m_attributeName);
    }

    if (attributeType == ATTRIBUTETYPE_XML || ret.isEmpty())
        ret = targetElement()->getAttribute(m_attributeName);

    return ret;
}

void SVGAnimationElement::setTargetAttribute(const String& value)
{
    SVGAnimationElement::setTargetAttribute(targetElement(), m_attributeName, value, static_cast<EAttributeType>(m_attributeType));
}

void SVGAnimationElement::setTargetAttribute(SVGElement* target, const String& name, const String& value, EAttributeType type)
{
    if (!target || name.isNull() || value.isNull())
        return;
    
    SVGStyledElement* styled = (target && target->isStyled()) ? static_cast<SVGStyledElement*>(target) : 0;

    EAttributeType attributeType = type;
    if (type == ATTRIBUTETYPE_AUTO) {
        // Spec: The implementation should match the attributeName to an attribute
        // for the target element. The implementation must first search through the
        // list of CSS properties for a matching property name, and if none is found,
        // search the default XML namespace for the element.
        if (styled && styled->style() && styled->style()->getPropertyCSSValue(name))
            attributeType = ATTRIBUTETYPE_CSS;
        else
            attributeType = ATTRIBUTETYPE_XML;
    }
    ExceptionCode ec = 0;
    if (attributeType == ATTRIBUTETYPE_CSS && styled && styled->style())
        styled->style()->setProperty(name, value, "", ec);
    else if (attributeType == ATTRIBUTETYPE_XML)
        target->setAttribute(name, value, ec);
}

String SVGAnimationElement::attributeName() const
{
    return m_attributeName;
}

bool SVGAnimationElement::connectedToTimer() const
{
    return m_connectedToTimer;
}

bool SVGAnimationElement::isFrozen() const
{
    return (m_fill == FILL_FREEZE);
}

bool SVGAnimationElement::isAdditive() const
{
    return (m_additive == ADDITIVE_SUM) || (detectAnimationMode() == BY_ANIMATION);
}

bool SVGAnimationElement::isAccumulated() const
{
    return (m_accumulate == ACCUMULATE_SUM) && (detectAnimationMode() != TO_ANIMATION);
}

EAnimationMode SVGAnimationElement::detectAnimationMode() const
{
    if ((!m_from.isEmpty() && !m_to.isEmpty()) || (!m_to.isEmpty())) { // to/from-to animation
        if (!m_from.isEmpty()) // from-to animation
            return FROM_TO_ANIMATION;
        else
            return TO_ANIMATION;
    } else if ((m_from.isEmpty() && m_to.isEmpty() && !m_by.isEmpty()) ||
            (!m_from.isEmpty() && !m_by.isEmpty())) { // by/from-by animation
        if (!m_from.isEmpty()) // from-by animation
            return FROM_BY_ANIMATION;
        else
            return BY_ANIMATION;
    }
    else if (m_values)
        return VALUES_ANIMATION;

    return NO_ANIMATION;
}

int SVGAnimationElement::calculateCurrentValueItem(double timePercentage)
{
    if (!m_values)
        return -1;
    
    unsigned long items = m_values->numberOfItems();

    // Calculate the relative time percentages for each 'fade'.
    Vector<double> startTimes(items);
    startTimes[0] = 0.0;
    for (unsigned i = 1; i < items; ++i)
        startTimes[i] = (((2.0 * i)) / (items - 1)) / 2.0;

    int itemByPercentage = -1;
    for (unsigned i = 0; i < items - 1; ++i) {
        if (timePercentage >= startTimes[i] && timePercentage <= startTimes[i + 1]) {
            itemByPercentage = i;
            break;
        }
    }

    return itemByPercentage;
}

double SVGAnimationElement::calculateRelativeTimePercentage(double timePercentage, int currentItem)
{
    if (currentItem == -1 || !m_values)
        return 0.0;

    unsigned long items = m_values->numberOfItems();

    // Calculate the relative time percentages for each 'fade'.
    Vector<double> startTimes(items);
    startTimes[0] = 0.0;
    for (unsigned i = 1; i < items; ++i)
        startTimes[i] = (((2.0 * i)) / (items - 1)) / 2.0;

    double beginTimePercentage = startTimes[currentItem];
    double endTimePercentage = startTimes[currentItem + 1];

    if ((endTimePercentage - beginTimePercentage) == 0.0)
        return 0.0;

    return ((timePercentage - beginTimePercentage) /
            (endTimePercentage - beginTimePercentage));
}

double SVGAnimationElement::repeations() const
{
    return m_repetitions;
}

bool SVGAnimationElement::isIndefinite(double value) const
{
    return (value == DBL_MAX);
}

void SVGAnimationElement::connectTimer()
{
    ASSERT(!m_connectedToTimer);
    ownerSVGElement()->timeScheduler()->connectIntervalTimer(this);
    m_connectedToTimer = true;
}

void SVGAnimationElement::disconnectTimer()
{
    ASSERT(m_connectedToTimer);
    ownerSVGElement()->timeScheduler()->disconnectIntervalTimer(this);
    m_connectedToTimer = false;
}

static double calculateTimePercentage(double elapsedSeconds, double start, double end, double duration, double repetitions)
{
    double percentage = 0.0;
    double useElapsed = elapsedSeconds - (duration * repetitions);
    
    if (duration > 0.0 && end == 0.0)
        percentage = 1.0 - (((start + duration) - useElapsed) / duration);
    else if (duration > 0.0 && end != 0.0) {
        if (duration > end)
            percentage = 1.0 - (((start + end) - useElapsed) / end);
        else
            percentage = 1.0 - (((start + duration) - useElapsed) / duration);
    } else if (duration == 0.0 && end != 0.0)
        percentage = 1.0 - (((start + end) - useElapsed) / end);
    
    return percentage;
}

void SVGAnimationElement::handleTimerEvent(double timePercentage)
{
    if (!connectedToTimer()) {
        connectTimer();
        handleStartCondition(); // Need to check bool return if adding anything after this call
        return;
    }
    
    if (!updateCurrentValue(timePercentage))
        return;
    
    if (timePercentage == 1.0) {
        if ((m_repeatCount > 0 && m_repetitions < m_repeatCount - 1) || isIndefinite(m_repeatCount)) {
            updateLastValueWithCurrent();
            m_repetitions++;
            return;
        }
        
        disconnectTimer();
        resetValues();   
    }
}

bool SVGAnimationElement::updateForElapsedSeconds(double elapsedSeconds)
{
    // Validate animation timing settings:
    // #1 (duration > 0) -> fine
    // #2 (duration <= 0.0 && end > 0) -> fine
    
    if ((m_simpleDuration <= 0.0 && m_end <= 0.0) || (isIndefinite(m_simpleDuration) && m_end <= 0.0))
        return false; // Ignore dur="0" or dur="-neg"
    
    float percentage = calculateTimePercentage(elapsedSeconds, m_begin, m_end, m_simpleDuration, m_repetitions);
    
    if (percentage <= 1.0 || connectedToTimer())
        handleTimerEvent(percentage);
    
    return true;
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT
