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
#include "SVGAnimateTransformElement.h"

#include "TimeScheduler.h"
#include "SVGAngle.h"
#include "AffineTransform.h"
#include "SVGSVGElement.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include <math.h>
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

SVGAnimateTransformElement::SVGAnimateTransformElement(const QualifiedName& tagName, Document* doc)
    : SVGAnimationElement(tagName, doc)
    , m_currentItem(-1)
    , m_type(SVGTransform::SVG_TRANSFORM_UNKNOWN)
    , m_rotateSpecialCase(false)
    , m_toRotateSpecialCase(false)
    , m_fromRotateSpecialCase(false)
{
}

SVGAnimateTransformElement::~SVGAnimateTransformElement()
{
}

bool SVGAnimateTransformElement::hasValidTarget() const
{
    return (SVGAnimationElement::hasValidTarget() && targetElement()->isStyledTransformable());
}

void SVGAnimateTransformElement::parseMappedAttribute(MappedAttribute* attr)
{
    if (attr->name() == SVGNames::typeAttr) {
        const String& value = attr->value();
        if (value == "translate")
            m_type = SVGTransform::SVG_TRANSFORM_TRANSLATE;
        else if (value == "scale")
            m_type = SVGTransform::SVG_TRANSFORM_SCALE;
        else if (value == "rotate")
            m_type = SVGTransform::SVG_TRANSFORM_ROTATE;
        else if (value == "skewX")
            m_type = SVGTransform::SVG_TRANSFORM_SKEWX;
        else if (value == "skewY")
            m_type = SVGTransform::SVG_TRANSFORM_SKEWY;
    } else
        SVGAnimationElement::parseMappedAttribute(attr);
}

void SVGAnimateTransformElement::storeInitialValue()
{
    m_initialTransform = 0;
    
    // Save initial transform... (needed for fill="remove" or additve="sum")
    if (targetElement()->isStyledTransformable()) {
        SVGStyledTransformableElement* transform = static_cast<SVGStyledTransformableElement*>(targetElement());
        RefPtr<SVGTransformList> transformList = transform->transformBaseValue();
        if (transformList) {
            ExceptionCode ec = 0;
            for (unsigned long i = 0; i < transformList->numberOfItems(); i++) {
                SVGTransform* value = transformList->getItem(i, ec).get();
                if (!value)
                    continue;
                
                // FIXME: This is wrong if the initial transform list has not been normalized
                if (value->type() == m_type) {
                    m_initialTransform = value;
                    break;
                }
            }
        }
    }
}

void SVGAnimateTransformElement::resetValues()
{
    m_currentItem = -1;
    m_toTransform = 0;
    m_fromTransform = 0;
    m_initialTransform = 0;
}

bool SVGAnimateTransformElement::updateCurrentValue(double timePercentage)
{
    AffineTransform qToMatrix, qFromMatrix;
    double useTimePercentage = timePercentage;
    
    if (m_values) {
        int itemByPercentage = calculateCurrentValueItem(timePercentage);
        
        if (itemByPercentage == -1)
            return false;
        
        if (m_currentItem != itemByPercentage) { // Item changed...
            ExceptionCode ec = 0;
            
            // Extract current 'from' / 'to' values
            String value1 = m_values->getItem(itemByPercentage, ec);
            String value2 = m_values->getItem(itemByPercentage + 1, ec);
            
            // Calculate new from/to transform values...
            if (!value1.isEmpty() && !value2.isEmpty()) {
                bool apply = false;
                if (m_toTransform && m_fromTransform) {    
                    qToMatrix = m_toTransform->matrix();
                    qFromMatrix = m_fromTransform->matrix();
                    
                    apply = true;
                    useTimePercentage = 1.0;
                }
                
                m_toTransform = parseTransformValue(value2);
                m_toRotateSpecialCase = m_rotateSpecialCase;
                
                m_fromTransform = parseTransformValue(value1);
                m_fromRotateSpecialCase = m_rotateSpecialCase;
                
                m_currentItem = itemByPercentage;
                
                if (!apply)
                    return false;
            }
        }
        else if (m_toTransform && m_fromTransform)
            useTimePercentage = calculateRelativeTimePercentage(timePercentage, m_currentItem);
    }
    
    if (m_toTransform && qToMatrix.isIdentity())
        qToMatrix = m_toTransform->matrix();
    
    if (m_fromTransform && qFromMatrix.isIdentity())
        qFromMatrix = m_fromTransform->matrix();
    
    m_currentTransform.reset();
    
    if (isAccumulated() && repeations() != 0.0)
        m_currentTransform.multiply(m_lastTransform);
    
    switch (m_type) {
        case SVGTransform::SVG_TRANSFORM_TRANSLATE:
        {
            double dx = ((qToMatrix.e() - qFromMatrix.e()) * useTimePercentage) + qFromMatrix.e();
            double dy = ((qToMatrix.f() - qFromMatrix.f()) * useTimePercentage) + qFromMatrix.f();
            
            m_currentTransform.translate(dx, dy);
            break;
        }
        case SVGTransform::SVG_TRANSFORM_SCALE:
        {
            double sx = ((qToMatrix.a() - qFromMatrix.a()) * useTimePercentage) + qFromMatrix.a();
            double sy = ((qToMatrix.d() - qFromMatrix.d()) * useTimePercentage) + qFromMatrix.d();
            
            m_currentTransform.scale(sx, sy);
            break;
        }
        case SVGTransform::SVG_TRANSFORM_ROTATE:
        {
            double toAngle, toCx, toCy, fromAngle, fromCx, fromCy;
            calculateRotationFromMatrix(qToMatrix, toAngle, toCx, toCy);
            calculateRotationFromMatrix(qFromMatrix, fromAngle, fromCx, fromCy);
            
            if (m_toRotateSpecialCase)
                toAngle = (lround(toAngle) == 1) ? 0.0 : 360.0;
            
            if (m_fromRotateSpecialCase)
                fromAngle = (lround(fromAngle) == 1) ? 0.0 : 360.0;
            
            double angle = ((toAngle - fromAngle) * useTimePercentage) + fromAngle;
            double cx = (toCx - fromCx) * useTimePercentage + fromCx;
            double cy = (toCy - fromCy) * useTimePercentage + fromCy;
            
            m_currentTransform.translate(cx, cy);
            m_currentTransform.rotate(angle);
            m_currentTransform.translate(-cx, -cy);
            break;
        }
        case SVGTransform::SVG_TRANSFORM_SKEWX:
        {
            double sx = (SVGAngle::todeg(atan(qToMatrix.c()) - atan(qFromMatrix.c())) *
                         useTimePercentage) + SVGAngle::todeg(atan(qFromMatrix.c()));
            
            m_currentTransform.shear(sx, 1.0);
            break;
        }
        case SVGTransform::SVG_TRANSFORM_SKEWY:
        {
            double sy = (SVGAngle::todeg(atan(qToMatrix.b()) - atan(qFromMatrix.b())) *
                         useTimePercentage) + SVGAngle::todeg(atan(qFromMatrix.b()));
            
            m_currentTransform.shear(1.0, sy);
            break;
        }
        default:
            break;
    }
    return true;
}

bool SVGAnimateTransformElement::handleStartCondition()
{
    storeInitialValue();
    
    switch (detectAnimationMode()) {
        case TO_ANIMATION:
        case FROM_TO_ANIMATION:
        {        
            m_toTransform = parseTransformValue(m_to);
            m_toRotateSpecialCase = m_rotateSpecialCase;
            
            if (!m_from.isEmpty()) { // from-to animation
                m_fromTransform = parseTransformValue(m_from);
                m_fromRotateSpecialCase = m_rotateSpecialCase;
            } else { // to animation
                m_fromTransform = m_initialTransform;
                m_fromRotateSpecialCase = false;
            }
            
            if (!m_fromTransform)
                m_fromTransform = new SVGTransform();
            
            break;
        }
        case BY_ANIMATION:
        case FROM_BY_ANIMATION:
        {
            m_toTransform = parseTransformValue(m_by);
            m_toRotateSpecialCase = m_rotateSpecialCase;
            
            if (!m_from.isEmpty()) { // from-by animation
                m_fromTransform = parseTransformValue(m_from);
                m_fromRotateSpecialCase = m_rotateSpecialCase;
            } else { // by animation
                m_fromTransform = m_initialTransform;
                m_fromRotateSpecialCase = false;
            }
            
            if (!m_fromTransform)
                m_fromTransform = new SVGTransform();
            break;
        }
        case VALUES_ANIMATION:
            break;
        default:
        {
            //kdError() << k_funcinfo << " Unable to detect animation mode! Aborting creation!" << endl;
            return false;
        }
    }
    return true;
}

void SVGAnimateTransformElement::updateLastValueWithCurrent()
{
    m_lastTransform = m_currentTransform;
}

void SVGAnimateTransformElement::applyAnimationToValue(SVGTransformList* targetTransforms)
{
    ExceptionCode ec;
    if (!isAdditive())
        targetTransforms->clear(ec);
    
    RefPtr<SVGTransform> targetTransform = new SVGTransform();
    targetTransform->setMatrix(m_currentTransform);
    targetTransforms->appendItem(targetTransform.get(), ec);
}

RefPtr<SVGTransform> SVGAnimateTransformElement::parseTransformValue(const String& data) const
{
    String parse = data.stripWhiteSpace();
    if (parse.isEmpty())
        return 0;
    
    int commaPos = parse.find(','); // In case two values are passed...

    RefPtr<SVGTransform> parsedTransform = new SVGTransform();
    
    switch (m_type) {
        case SVGTransform::SVG_TRANSFORM_TRANSLATE:
        {
            double tx = 0.0, ty = 0.0;
            if (commaPos != - 1) {
                tx = parse.substring(0, commaPos).toDouble();
                ty = parse.substring(commaPos + 1).toDouble();
            } else 
                tx = parse.toDouble();

            parsedTransform->setTranslate(tx, ty);
            break;
        }
        case SVGTransform::SVG_TRANSFORM_SCALE:
        {
            double sx = 1.0, sy = 1.0;
            if (commaPos != - 1) {
                sx = parse.substring(0, commaPos).toDouble();
                sy = parse.substring(commaPos + 1).toDouble();
            } else {
                sx = parse.toDouble();
                sy = sx;
            }

            parsedTransform->setScale(sx, sy);
            break;
        }
        case SVGTransform::SVG_TRANSFORM_ROTATE:
        {
            double angle = 0, cx = 0, cy = 0;
            if (commaPos != - 1) {
                angle = parse.substring(0, commaPos).toDouble(); // TODO: probably needs it's own 'angle' parser
    
                int commaPosTwo = parse.find(',', commaPos + 1); // In case three values are passed...
                if (commaPosTwo != -1) {
                    cx = parse.substring(commaPos + 1, commaPosTwo - commaPos - 1).toDouble();
                    cy = parse.substring(commaPosTwo + 1).toDouble();
                }
            }
            else 
                angle = parse.toDouble();

            // Ok now here's a hack to make it possible to calculate cx/cy values, if angle = 0 
            // or angle=360 -> for angle=0 our matrix is m11: 1 m12: 0 m21: 0 m22: 1 dx: 0 dy: 0
            // As you can see there is no way to retrieve the cx/cy values for these angles.
            // -> set 'm_rotateSpecialCase' to true, and save angle = 1/359 -> this way we can calculate
            //    the cx/cy values, while keeping this uber-optimized way of handling <animateTransform>!
            m_rotateSpecialCase = false;
            
            if (angle == 0.0) {
                angle = angle + 1.0;
                m_rotateSpecialCase = true;
            } else if (angle == 360.0) {
                angle = angle - 1.0;
                m_rotateSpecialCase = true;
            }
            
            parsedTransform->setRotate(angle, cx, cy);
            break;    
        }
        case SVGTransform::SVG_TRANSFORM_SKEWX:
        case SVGTransform::SVG_TRANSFORM_SKEWY:
        {
            double angle = parse.toDouble(); // TODO: probably needs it's own 'angle' parser
            
            if (m_type == SVGTransform::SVG_TRANSFORM_SKEWX)
                parsedTransform->setSkewX(angle);
            else
                parsedTransform->setSkewY(angle);

            break;
        }
        default:
            return 0;
    }
    
    return parsedTransform;
}

void SVGAnimateTransformElement::calculateRotationFromMatrix(const AffineTransform& matrix, double& angle, double& cx, double& cy) const
{
    double cosa = matrix.a();
    double sina = -matrix.c();

    if (cosa != 1.0) {
        // Calculate the angle via magic ;)
        double temp = SVGAngle::todeg(asin(sina));
        angle = SVGAngle::todeg(acos(cosa));
        if (temp < 0)
            angle = 360.0 - angle;
        
        double res = (1 - cosa) + ((sina * sina) / (1 - cosa));
        
        cx = (matrix.e() - ((sina * matrix.f()) / (1 - cosa))) / res;
        cy = (matrix.f() + ((sina * matrix.e()) / (1 - cosa))) / res;

        return;
    }

    cx = 0.0;
    cy = 0.0;
    angle = 0.0;
}

AffineTransform SVGAnimateTransformElement::currentTransform() const
{
    return m_currentTransform;
}

}

// vim:ts=4:noet
#endif // SVG_SUPPORT
