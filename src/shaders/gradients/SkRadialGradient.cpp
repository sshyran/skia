/*
 * Copyright 2012 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/shaders/gradients/SkRadialGradient.h"

#include "src/core/SkRasterPipeline.h"
#include "src/core/SkReadBuffer.h"
#include "src/core/SkWriteBuffer.h"

#ifdef SK_ENABLE_SKSL
#include "src/core/SkKeyHelpers.h"
#include "src/core/SkPaintParamsKey.h"
#endif

namespace {

SkMatrix rad_to_unit_matrix(const SkPoint& center, SkScalar radius) {
    SkScalar    inv = SkScalarInvert(radius);

    SkMatrix matrix;
    matrix.setTranslate(-center.fX, -center.fY);
    matrix.postScale(inv, inv);
    return matrix;
}

}  // namespace

/////////////////////////////////////////////////////////////////////

SkRadialGradient::SkRadialGradient(const SkPoint& center, SkScalar radius, const Descriptor& desc)
    : SkGradientShaderBase(desc, rad_to_unit_matrix(center, radius))
    , fCenter(center)
    , fRadius(radius) {
}

SkShader::GradientType SkRadialGradient::asAGradient(GradientInfo* info) const {
    if (info) {
        commonAsAGradient(info);
        info->fPoint[0] = fCenter;
        info->fRadius[0] = fRadius;
    }
    return kRadial_GradientType;
}

sk_sp<SkFlattenable> SkRadialGradient::CreateProc(SkReadBuffer& buffer) {
    DescriptorScope desc;
    if (!desc.unflatten(buffer)) {
        return nullptr;
    }
    const SkPoint center = buffer.readPoint();
    const SkScalar radius = buffer.readScalar();
    return SkGradientShader::MakeRadial(center, radius, desc.fColors, std::move(desc.fColorSpace),
                                        desc.fPos, desc.fCount, desc.fTileMode, desc.fGradFlags,
                                        desc.fLocalMatrix);
}

void SkRadialGradient::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writePoint(fCenter);
    buffer.writeScalar(fRadius);
}

void SkRadialGradient::appendGradientStages(SkArenaAlloc*, SkRasterPipeline* p,
                                            SkRasterPipeline*) const {
    p->append(SkRasterPipeline::xy_to_radius);
}

skvm::F32 SkRadialGradient::transformT(skvm::Builder* p, skvm::Uniforms*,
                                       skvm::Coord coord, skvm::I32* mask) const {
    return sqrt(coord.x*coord.x + coord.y*coord.y);
}

/////////////////////////////////////////////////////////////////////

#if SK_SUPPORT_GPU

#include "src/gpu/ganesh/gradients/GrGradientShader.h"

std::unique_ptr<GrFragmentProcessor> SkRadialGradient::asFragmentProcessor(
        const GrFPArgs& args) const {
    return GrGradientShader::MakeRadial(*this, args);
}

#endif

#ifdef SK_ENABLE_SKSL
void SkRadialGradient::addToKey(const SkKeyContext& keyContext,
                                SkPaintParamsKeyBuilder* builder,
                                SkPipelineDataGatherer* gatherer) const {
    GradientShaderBlocks::GradientData data(kRadial_GradientType,
                                            SkM44(this->getLocalMatrix()),
                                            fCenter, { 0.0f, 0.0f },
                                            fRadius, 0.0f,
                                            0.0f, 0.0f,
                                            fTileMode,
                                            fColorCount,
                                            fOrigColors4f,
                                            fOrigPos);

    GradientShaderBlocks::BeginBlock(keyContext, builder, gatherer, data);
    builder->endBlock();
}
#endif
