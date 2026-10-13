// The SIMD inverse of GLM (6.2). It only exists with GLM_FORCE_INTRINSICS, and that define changes
// every GLM type: they stop being literal, and a constexpr pxVec3 no longer compiles. So it lives in
// this file alone, which takes only GLM's setup and its SIMD functions, on __m128, and no GLM type:
// nothing here can disagree with the GLM the other files see.
#define GLM_FORCE_INTRINSICS
#include <glm/detail/setup.hpp>

#if defined(__x86_64__) || defined(_M_X64)

#include <glm/simd/matrix.h>

static_assert(GLM_ARCH & GLM_ARCH_SSE2_BIT, "pxInverse expects GLM's SSE2 functions on x86-64");

namespace pxMathDetail {

// The columns of a pxMat4f, which has no alignment beyond a float's: loaded and stored unaligned.
void InverseSimd(const float* in, float* out) noexcept {
    glm_vec4 m[4];
    glm_vec4 inverse[4];
    for (int c = 0; c < 4; ++c)
        m[c] = _mm_loadu_ps(in + 4 * c);
    glm_mat4_inverse(m, inverse);
    for (int c = 0; c < 4; ++c)
        _mm_storeu_ps(out + 4 * c, inverse[c]);
}

}  // namespace pxMathDetail

#endif
