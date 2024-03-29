/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef FEGaussianBlur_h
#define FEGaussianBlur_h

#include "FEConvolveMatrix.h"
#include "Filter.h"
#include "FilterEffect.h"

namespace WebCore {

class FEGaussianBlur : public FilterEffect {
public:
    static Ref<FEGaussianBlur> create(Filter*, float, float, EdgeModeType);

    float stdDeviationX() const;
    void setStdDeviationX(float);

    float stdDeviationY() const;
    void setStdDeviationY(float);

    EdgeModeType edgeMode() const;
    void setEdgeMode(EdgeModeType);

    virtual void platformApplySoftware();
    virtual void dump();

    virtual void determineAbsolutePaintRect();
    static IntSize calculateKernelSize(const Filter&, const FloatPoint& stdDeviation);
    static IntSize calculateUnscaledKernelSize(const FloatPoint& stdDeviation);

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    static const int s_minimalRectDimension = 100 * 100; // Empirical data limit for parallel jobs

    template<typename Type>
    friend class ParallelJobs;

    struct PlatformApplyParameters {
        FEGaussianBlur* filter;
        RefPtr<Uint8ClampedArray> srcPixelArray;
        RefPtr<Uint8ClampedArray> dstPixelArray;
        int width;
        int height;
        unsigned kernelSizeX;
        unsigned kernelSizeY;
    };

    static void platformApplyWorker(PlatformApplyParameters*);

    FEGaussianBlur(Filter*, float, float, EdgeModeType);

    inline void platformApply(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize);

    inline void platformApplyGeneric(Uint8ClampedArray* srcPixelArray, Uint8ClampedArray* tmpPixelArray, unsigned kernelSizeX, unsigned kernelSizeY, IntSize& paintSize);

    float m_stdX;
    float m_stdY;
    EdgeModeType m_edgeMode;
};

} // namespace WebCore

#endif // FEGaussianBlur_h
