
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef SkDraw_DEFINED
#define SkDraw_DEFINED

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkStrokeRec.h"
#include "src/core/SkGlyphRunPainter.h"
#include "src/core/SkMask.h"

class SkBitmap;
class SkClipStack;
class SkBaseDevice;
class SkBlitter;
class SkMatrix;
class SkMatrixProvider;
class SkPath;
class SkRegion;
class SkRasterClip;
struct SkRect;
class SkRRect;
class SkVertices;

class SkDraw : public SkGlyphRunListPainterCPU::BitmapDevicePainter {
public:
    SkDraw();

    void    drawPaint(const SkPaint&) const;
    void    drawPoints(SkCanvas::PointMode, size_t count, const SkPoint[],
                       const SkPaint&, SkBaseDevice*) const;
    void    drawRect(const SkRect& prePaintRect, const SkPaint&, const SkMatrix* paintMatrix,
                     const SkRect* postPaintRect) const;
    void    drawRect(const SkRect& rect, const SkPaint& paint) const {
        this->drawRect(rect, paint, nullptr, nullptr);
    }
    void    drawRRect(const SkRRect&, const SkPaint&) const;
    /**
     *  To save on mallocs, we allow a flag that tells us that srcPath is
     *  mutable, so that we don't have to make copies of it as we transform it.
     *
     *  If prePathMatrix is not null, it should logically be applied before any
     *  stroking or other effects. If there are no effects on the paint that
     *  affect the geometry/rasterization, then the pre matrix can just be
     *  pre-concated with the current matrix.
     */
    void    drawPath(const SkPath& path, const SkPaint& paint,
                     const SkMatrix* prePathMatrix = nullptr, bool pathIsMutable = false) const {
        this->drawPath(path, paint, prePathMatrix, pathIsMutable, false);
    }

    /* If dstOrNull is null, computes a dst by mapping the bitmap's bounds through the matrix. */
    void    drawBitmap(const SkBitmap&, const SkMatrix&, const SkRect* dstOrNull,
                       const SkSamplingOptions&, const SkPaint&) const override;
    void    drawSprite(const SkBitmap&, int x, int y, const SkPaint&) const;
    void    drawGlyphRunList(SkCanvas* canvas,
                             SkGlyphRunListPainterCPU* glyphPainter,
                             const SkGlyphRunList& glyphRunList,
                             const SkPaint& paint) const;
    /* If skipColorXform, skips color conversion when assigning per-vertex colors */
    void drawVertices(const SkVertices*,
                      sk_sp<SkBlender>,
                      const SkPaint&,
                      bool skipColorXform) const;
    void  drawAtlas(const SkRSXform[], const SkRect[], const SkColor[], int count,
                    sk_sp<SkBlender>, const SkPaint&);

    /**
     *  Overwrite the target with the path's coverage (i.e. its mask).
     *  Will overwrite the entire device, so it need not be zero'd first.
     *
     *  Only device A8 is supported right now.
     */
    void drawPathCoverage(const SkPath& src, const SkPaint& paint,
                          SkBlitter* customBlitter = nullptr) const {
        bool isHairline = paint.getStyle() == SkPaint::kStroke_Style &&
                          paint.getStrokeWidth() > 0;
        this->drawPath(src, paint, nullptr, false, !isHairline, customBlitter);
    }

    void paintMasks(SkDrawableGlyphBuffer* accepted, const SkPaint& paint) const override;

    static bool ComputeMaskBounds(const SkRect& devPathBounds, const SkIRect& clipBounds,
                                  const SkMaskFilter* filter, const SkMatrix* filterMatrix,
                                  SkIRect* bounds);

    /** Helper function that creates a mask from a path and an optional maskfilter.
        Note however, that the resulting mask will not have been actually filtered,
        that must be done afterwards (by calling filterMask). The maskfilter is provided
        solely to assist in computing the mask's bounds (if the mode requests that).
    */
    static bool DrawToMask(const SkPath& devPath, const SkIRect& clipBounds,
                           const SkMaskFilter*, const SkMatrix* filterMatrix,
                           SkMask* mask, SkMask::CreateMode mode,
                           SkStrokeRec::InitStyle style);

    enum RectType {
        kHair_RectType,
        kFill_RectType,
        kStroke_RectType,
        kPath_RectType
    };

    /**
     *  Based on the paint's style, strokeWidth, and the matrix, classify how
     *  to draw the rect. If no special-case is available, returns
     *  kPath_RectType.
     *
     *  Iff RectType == kStroke_RectType, then strokeSize is set to the device
     *  width and height of the stroke.
     */
    static RectType ComputeRectType(const SkRect&, const SkPaint&, const SkMatrix&,
                                    SkPoint* strokeSize);

private:
    void drawFixedVertices(const SkVertices* vertices,
                           sk_sp<SkBlender> blender,
                           const SkPaint& paint,
                           const SkMatrix& ctmInverse,
                           const SkPoint* dev2,
                           const SkPoint3* dev3,
                           SkArenaAlloc* outerAlloc,
                           bool skipColorXform) const;

    void drawPath(const SkPath&,
                  const SkPaint&,
                  const SkMatrix* preMatrix,
                  bool pathIsMutable,
                  bool drawCoverage,
                  SkBlitter* customBlitter = nullptr) const;

    void drawLine(const SkPoint[2], const SkPaint&) const;

    void drawDevPath(const SkPath& devPath,
                     const SkPaint& paint,
                     bool drawCoverage,
                     SkBlitter* customBlitter,
                     bool doFill) const;
    /**
     *  Return the current clip bounds, in local coordinates, with slop to account
     *  for antialiasing or hairlines (i.e. device-bounds outset by 1, and then
     *  run through the inverse of the matrix).
     *
     *  If the matrix cannot be inverted, or the current clip is empty, return
     *  false and ignore bounds parameter.
     */
    bool SK_WARN_UNUSED_RESULT computeConservativeLocalClipBounds(SkRect* bounds) const;

public:
    SkPixmap                fDst;
    const SkMatrixProvider* fMatrixProvider{nullptr};  // required
    const SkRasterClip*     fRC{nullptr};              // required

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif
};

#endif
