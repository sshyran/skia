/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_UTIL
#define SKSL_UTIL

#include "include/sksl/SkSLVersion.h"
#include "src/core/SkSLTypeShared.h"
#include "src/sksl/SkSLGLSL.h"

#include <memory>

#ifndef SKSL_STANDALONE
#include "include/core/SkTypes.h"
#endif // SKSL_STANDALONE

namespace SkSL {

class Context;
class OutputStream;
class StringStream;
class Type;

struct ShaderCaps {
    /**
     * Indicates how GLSL must interact with advanced blend equations. The KHR extension requires
     * special layout qualifiers in the fragment shader.
     */
    enum AdvBlendEqInteraction {
        kNotSupported_AdvBlendEqInteraction,     //<! No _blend_equation_advanced extension
        kAutomatic_AdvBlendEqInteraction,        //<! No interaction required
        kGeneralEnable_AdvBlendEqInteraction,    //<! layout(blend_support_all_equations) out

        kLast_AdvBlendEqInteraction = kGeneralEnable_AdvBlendEqInteraction
    };

    bool mustEnableAdvBlendEqs() const {
        return fAdvBlendEqInteraction >= kGeneralEnable_AdvBlendEqInteraction;
    }

    bool mustDeclareFragmentShaderOutput() const {
        return fGLSLGeneration > SkSL::GLSLGeneration::k110;
    }

    // Returns the string of an extension that must be enabled in the shader to support
    // derivatives. If nullptr is returned then no extension needs to be enabled. Before calling
    // this function, the caller should check that shaderDerivativeSupport exists.
    const char* shaderDerivativeExtensionString() const {
        SkASSERT(this->fShaderDerivativeSupport);
        return fShaderDerivativeExtensionString;
    }

    // This returns the name of an extension that must be enabled in the shader to support external
    // textures. In some cases, two extensions must be enabled - the second extension is returned
    // by secondExternalTextureExtensionString(). If that function returns nullptr, then only one
    // extension is required.
    const char* externalTextureExtensionString() const {
        SkASSERT(this->fExternalTextureSupport);
        return fExternalTextureExtensionString;
    }

    const char* secondExternalTextureExtensionString() const {
        SkASSERT(this->fExternalTextureSupport);
        return fSecondExternalTextureExtensionString;
    }

    /**
     * SkSL 300 requires support for derivatives, nonsquare matrices and bitwise integer operations.
     */
    SkSL::Version supportedSkSLVerion() const {
        if (fShaderDerivativeSupport && fNonsquareMatrixSupport && fIntegerSupport &&
            fGLSLGeneration >= SkSL::GLSLGeneration::k330) {
            return SkSL::Version::k300;
        }
        return SkSL::Version::k100;
    }

    SkSL::GLSLGeneration fGLSLGeneration = SkSL::GLSLGeneration::k330;

    bool fShaderDerivativeSupport = false;
    /** Indicates true 32-bit integer support, with unsigned types and bitwise operations */
    bool fIntegerSupport = false;
    bool fNonsquareMatrixSupport = false;
    /** asinh(), acosh(), atanh() */
    bool fInverseHyperbolicSupport = false;
    bool fFBFetchSupport = false;
    bool fFBFetchNeedsCustomOutput = false;
    bool fUsesPrecisionModifiers = false;
    bool fFlatInterpolationSupport = false;
    bool fNoPerspectiveInterpolationSupport = false;
    bool fSampleMaskSupport = false;
    bool fExternalTextureSupport = false;
    bool fFloatIs32Bits = true;

    // Used by SkSL to know when to generate polyfills.
    bool fBuiltinFMASupport = false;
    bool fBuiltinDeterminantSupport = false;

    // Used for specific driver bug work arounds
    bool fCanUseMinAndAbsTogether = true;
    bool fCanUseFractForNegativeValues = true;
    bool fMustForceNegatedAtanParamToFloat = false;
    bool fMustForceNegatedLdexpParamToMultiply = false;  // http://skbug.com/12076
    // Returns whether a device incorrectly implements atan(y,x) as atan(y/x)
    bool fAtan2ImplementedAsAtanYOverX = false;
    // If this returns true some operation (could be a no op) must be called between floor and abs
    // to make sure the driver compiler doesn't inline them together which can cause a driver bug in
    // the shader.
    bool fMustDoOpBetweenFloorAndAbs = false;
    // The D3D shader compiler, when targeting PS 3.0 (ie within ANGLE) fails to compile certain
    // constructs. See detailed comments in GrGLCaps.cpp.
    bool fMustGuardDivisionEvenAfterExplicitZeroCheck = false;
    // If false, SkSL uses a workaround so that sk_FragCoord doesn't actually query gl_FragCoord
    bool fCanUseFragCoord = true;
    // If true, short ints can't represent every integer in the 16-bit two's complement range as
    // required by the spec. SKSL will always emit full ints.
    bool fIncompleteShortIntPrecision = false;
    // If true, then conditions in for loops need "&& true" to work around driver bugs.
    bool fAddAndTrueToLoopCondition = false;
    // If true, then expressions such as "x && y" or "x || y" are rewritten as ternary to work
    // around driver bugs.
    bool fUnfoldShortCircuitAsTernary = false;
    bool fEmulateAbsIntFunction = false;
    bool fRewriteDoWhileLoops = false;
    bool fRewriteSwitchStatements = false;
    bool fRemovePowWithConstantExponent = false;
    // The Android emulator claims samplerExternalOES is an unknown type if a default precision
    // statement is made for the type.
    bool fNoDefaultPrecisionForExternalSamplers = false;
    // ARM GPUs calculate `matrix * vector` in SPIR-V at full precision, even when the inputs are
    // RelaxedPrecision. Rewriting the multiply as a sum of vector*scalar fixes this. (skia:11769)
    bool fRewriteMatrixVectorMultiply = false;
    // Rewrites matrix equality comparisons to avoid an Adreno driver bug. (skia:11308)
    bool fRewriteMatrixComparisons = false;

    // This controls behavior of the SkSL compiler, not the code we generate. By default, SkSL pools
    // IR nodes per-program. To debug memory corruption, it can be helpful to disable that feature.
    bool fUseNodePools = true;

    const char* fVersionDeclString = "";

    const char* fShaderDerivativeExtensionString = nullptr;
    const char* fExternalTextureExtensionString = nullptr;
    const char* fSecondExternalTextureExtensionString = nullptr;
    const char* fFBFetchColorName = nullptr;

    AdvBlendEqInteraction fAdvBlendEqInteraction = kNotSupported_AdvBlendEqInteraction;
};

// Various sets of caps for use in tests
class ShaderCapsFactory {
public:
    static std::unique_ptr<ShaderCaps> Default() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fShaderDerivativeSupport = true;
        result->fBuiltinDeterminantSupport = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> Standalone() {
        return MakeShaderCaps();
    }

    static std::unique_ptr<ShaderCaps> AddAndTrueToLoopCondition() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fAddAndTrueToLoopCondition = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> CannotUseFractForNegativeValues() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fCanUseFractForNegativeValues = false;
        return result;
    }

    static std::unique_ptr<ShaderCaps> CannotUseFragCoord() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fCanUseFragCoord = false;
        return result;
    }

    static std::unique_ptr<ShaderCaps> CannotUseMinAndAbsTogether() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fCanUseMinAndAbsTogether = false;
        return result;
    }

    static std::unique_ptr<ShaderCaps> EmulateAbsIntFunction() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fEmulateAbsIntFunction = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> FramebufferFetchSupport() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fFBFetchSupport = true;
        result->fFBFetchColorName = "gl_LastFragData[0]";
        return result;
    }

    static std::unique_ptr<ShaderCaps> IncompleteShortIntPrecision() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 310es";
        result->fUsesPrecisionModifiers = true;
        result->fIncompleteShortIntPrecision = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> MustForceNegatedAtanParamToFloat() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fMustForceNegatedAtanParamToFloat = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> MustForceNegatedLdexpParamToMultiply() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fMustForceNegatedLdexpParamToMultiply = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> MustGuardDivisionEvenAfterExplicitZeroCheck() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fMustGuardDivisionEvenAfterExplicitZeroCheck = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> RemovePowWithConstantExponent() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fRemovePowWithConstantExponent = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> RewriteDoWhileLoops() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fRewriteDoWhileLoops = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> RewriteMatrixComparisons() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fRewriteMatrixComparisons = true;
        result->fUsesPrecisionModifiers = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> RewriteMatrixVectorMultiply() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fRewriteMatrixVectorMultiply = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> RewriteSwitchStatements() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fRewriteSwitchStatements = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> SampleMaskSupport() {
        std::unique_ptr<ShaderCaps> result = Default();
        result->fSampleMaskSupport = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> ShaderDerivativeExtensionString() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fShaderDerivativeSupport = true;
        result->fShaderDerivativeExtensionString = "GL_OES_standard_derivatives";
        result->fUsesPrecisionModifiers = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> UnfoldShortCircuitAsTernary() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fUnfoldShortCircuitAsTernary = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> UsesPrecisionModifiers() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 400";
        result->fUsesPrecisionModifiers = true;
        return result;
    }

    static std::unique_ptr<ShaderCaps> Version110() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 110";
        result->fGLSLGeneration = SkSL::GLSLGeneration::k110;
        return result;
    }

    static std::unique_ptr<ShaderCaps> Version450Core() {
        std::unique_ptr<ShaderCaps> result = MakeShaderCaps();
        result->fVersionDeclString = "#version 450 core";
        return result;
    }

private:
    static std::unique_ptr<ShaderCaps> MakeShaderCaps();
};

#if !defined(SKSL_STANDALONE) && (SK_SUPPORT_GPU || defined(SK_GRAPHITE_ENABLED))
bool type_to_sksltype(const Context& context, const Type& type, SkSLType* outType);
#endif

void write_stringstream(const StringStream& d, OutputStream& out);

}  // namespace SkSL

#endif  // SKSL_UTIL
