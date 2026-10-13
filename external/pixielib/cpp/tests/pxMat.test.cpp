#include <cstring>
#include <numbers>
#include <type_traits>

#include <pixie/math/pxMat.hpp>

#include "check.hpp"

namespace {

template<typename M, typename K>
concept Multipliable = requires(M m, K k) { m * k; };

template<typename M>
bool SameBits(const M& a, const M& b) {
    return std::memcmp(&a, &b, sizeof(M)) == 0;
}

// A matrix with no special values, used by the C# tests too.
const pxMat4 M(pxVec4(0.1, 0.7, -1.3, 2.2), pxVec4(1.9, -0.4, 0.6, 0.05), pxVec4(-0.8, 1.1, 0.3, -2.7),
               pxVec4(0.25, -1.6, 2.4, 0.9));
const pxMat3 N(pxVec3(0.1, 0.7, -1.3), pxVec3(1.9, -0.4, 0.6), pxVec3(-0.8, 1.1, 0.3));

}  // namespace

void TestMat() {
    const pxMat4f Mf(M);
    const pxMat3f Nf(N);

    // A matrix starts as the identity (GLM_FORCE_CTOR_INIT, from pixie::math); in C# it is zero.
    PX_CHECK(pxMat4() == pxMat4(1.0) && pxMat3f() == pxMat3f(1.0f));
    PX_CHECK(pxMat4(2.0)[1] == pxVec4(0, 2, 0, 0));

    // Column-major and M * v (1.12): m[3] is the fourth column, and in T * S * v the S comes first.
    pxMat4 t = glm::translate(pxMat4(1.0), pxVec3(1, 2, 3));
    pxMat4 s = glm::scale(pxMat4(1.0), pxVec3(2, 2, 2));
    PX_CHECK(t[3] == pxVec4(1, 2, 3, 1));
    PX_CHECK(t * s * pxVec4(1, 1, 1, 1) == pxVec4(3, 4, 5, 1));
    PX_CHECK(s * t * pxVec4(1, 1, 1, 1) == pxVec4(4, 6, 8, 1));
    PX_CHECK(glm::transpose(M)[0] == pxVec4(0.1, 1.9, -0.8, 0.25));

    // The order of the sums is GLM's, and the C# tests expect it: M * v adds the columns in pairs,
    // M1 * M2 and the mat3 add them in sequence. With 1e16, adding 1 is lost, so each order gives
    // another number.
    pxMat4 a(pxVec4(1e16, 0, 0, 0), pxVec4(1, 0, 0, 0), pxVec4(-1e16, 0, 0, 0), pxVec4(1, 0, 0, 0));
    pxVec4 one(1);
    PX_CHECK((a * one).x == 0);
    PX_CHECK((a * pxMat4(one, one, one, one))[0].x == 1);
    pxMat3 b(pxVec3(1, 0, 0), pxVec3(1e16, 0, 0), pxVec3(-1e16, 0, 0));
    PX_CHECK((b * pxVec3(1)).x == 0 && (b * pxMat3(pxVec3(1), pxVec3(1), pxVec3(1)))[0].x == 0);
    // v * M is a dot product per column, and GLM's vec4 dot adds in pairs, except on MSVC, where it
    // adds in sequence (docs/layout.md, 3).
#if defined(_MSC_VER)
    PX_CHECK((one * glm::transpose(a)).x == 1);
#else
    PX_CHECK((one * glm::transpose(a)).x == 0);
#endif

    // The same bits as the C# tests.
    PX_CHECK(pxToString(M * glm::transpose(M)) ==
             "((4.3225, -1.9700000000000002, 1.3699999999999997, 2.7), "
             "(-1.9700000000000002, 4.420000000000001, -4.66, -2.8900000000000006), "
             "(1.3699999999999997, -4.66, 7.9, -1.4800000000000004), "
             "(2.7, -2.8900000000000006, -1.4800000000000004, 12.942500000000003))");
    PX_CHECK(pxToString(Mf * glm::transpose(Mf)) ==
             "((4.3224998, -1.97, 1.37, 2.7), (-1.97, 4.42, -4.66, -2.8899999), "
             "(1.37, -4.66, 7.8999996, -1.4799998), (2.7, -2.8899999, -1.4799998, 12.942501))");
    PX_CHECK(pxToString(M * pxVec4(0.3, -1.7, 2.9, 0.6)) == "(-5.37, 3.12, 0.8999999999999999, -6.715)");
    PX_CHECK(pxToString(Mf * pxVec4f(0.3f, -1.7f, 2.9f, 0.6f)) == "(-5.37, 3.12, 0.9000001, -6.715)");
    PX_CHECK(pxToString(N * glm::transpose(N)) ==
             "((4.26, -1.5700000000000003, 0.7699999999999998), (-1.5700000000000003, 1.86, "
             "-0.8199999999999998), (0.7699999999999998, -0.8199999999999998, 2.14))");
    PX_CHECK(pxToString(N * pxVec3(0.3, -1.7, 2.9)) == "(-5.52, 4.08, -0.5400000000000001)");
    PX_CHECK(pxToString(pxVec3(0.3, -1.7, 2.9) * N) == "(-4.93, 2.99, -1.2400000000000002)");

    // The inverse and the determinant.
    PX_CHECK(pxToString(pxInverse(M)) ==
             "((0.040714062010648304, 0.5413628025591696, 0.005219751539826708, -0.1139397193265029), "
             "(0.9509082367679297, 0.28130732405708914, 0.891142081636914, 0.33335570369707546), "
             "(0.46938615721891636, 0.14238736521855852, 0.5729982252844765, 0.5636958823617139), "
             "(0.4274976511118071, -0.02997628741443332, 0.05480739116818045, 0.23220437564314794))");
    PX_CHECK(pxToString(pxInverse(Mf)) ==
             "((0.040714066, 0.54136276, 0.0052197482, -0.1139397), "
             "(0.95090824, 0.2813073, 0.8911419, 0.33335567), "
             "(0.4693861, 0.14238736, 0.57299817, 0.5636958), "
             "(0.42749763, -0.029976288, 0.054807395, 0.23220432))");
    PX_CHECK(pxToString(pxInverse(N)) ==
             "((0.2504816955684008, 0.5266538214515094, 0.032113037893384724), "
             "(0.33718689788053946, 0.32434168272318564, 0.8124598587026333), "
             "(-0.5684007707129094, 0.21515735388567758, 0.43994861913937056))");
    PX_CHECK(pxToString(pxInverse(Nf)) ==
             "((0.2504817, 0.52665377, 0.032113027), (0.33718687, 0.32434165, 0.81245977), "
             "(-0.5684007, 0.21515734, 0.43994856))");
    PX_CHECK(glm::determinant(M) == -13.4106 && glm::determinant(Mf) == -13.410602f);
    PX_CHECK(glm::determinant(N) == -3.114 && glm::determinant(Nf) == -3.114f);

    // A matrix whose determinant and inverse change with the order of the last sums: GLM adds the
    // determinant in sequence and, in the inverse, in pairs.
    pxMat4 p(pxVec4(-1.6, -1.6, -0.1, -3), pxVec4(-3, -1.4, -2.9, 2.7), pxVec4(-0.2, -1.4, -2.1, -2.6),
             pxVec4(1.1, -1.4, 1.8, 3));
    pxMat4f pf(p);
    PX_CHECK(glm::determinant(p) == 121.3586 && glm::determinant(pf) == 121.358604f);
    PX_CHECK(pxToString(pxInverse(p)) ==
             "((-0.29013189011738755, -0.14986988973175364, 0.3410883118295695, 0.14036088089348428), "
             "(-0.13743566586957992, -0.02498380831683952, -0.22354410812253928, -0.30868846542395845), "
             "(0.2603688572544508, -0.08816845283317373, -0.29416951085460774, 0.08477355539698049), "
             "(-0.113976265382099, 0.09619425405368882, -0.05288459161526252, 0.08694892656968686))");
    PX_CHECK(pxToString(pxInverse(pf)) ==
             "((-0.29013187, -0.1498699, 0.34108835, 0.14036086), "
             "(-0.13743566, -0.024983808, -0.22354412, -0.30868843), "
             "(0.26036882, -0.08816845, -0.29416952, 0.08477355), "
             "(-0.11397627, 0.09619425, -0.052884597, 0.08694892))");

    // pxInverse is glm::inverse: the SIMD of the float mat4 gives the same bits.
    for (int i = 0; i < 64; ++i) {
        pxMat4f m;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                m[c][r] = static_cast<float>((i * 37 + c * 11 + r * 5) % 23) * 0.37f - 4.1f;
        PX_CHECK(SameBits(pxInverse(m), glm::inverse(m)));
    }
    PX_CHECK(SameBits(pxInverse(M), glm::inverse(M)));

    // The transforms. With Y down, as the 2D of PixieLib (1.13), a positive angle turns clockwise on
    // the screen: the x axis goes to (0, 1), which points down.
    double pi = std::numbers::pi;
    float pif = std::numbers::pi_v<float>;
    PX_CHECK(pxToString(glm::rotate(pxMat4(1.0), pi / 2, pxVec3(0, 0, 1)) * pxVec4(1, 0, 0, 0)) ==
             "(0.00000000000000006123233995736766, 1, 0, 0)");
    PX_CHECK(pxToString(glm::rotate(M, pi / 2, pxVec3(1, 2, 3))) ==
             "((2.058149923103555, -0.6801167939308144, 0.3778563474377664, 1.0690141783282412), "
             "(-0.07970165181795241, 0.1898816152304328, 1.2367829303178968, -3.3141009783569753), "
             "(-0.13291553982255017, 1.1667845211566497, -0.6838074026911867, -0.08027074053809691), "
             "(0.25, -1.6, 2.4, 0.9))");
    PX_CHECK(pxToString(glm::rotate(Mf, pif / 2, pxVec3f(1, 2, 3))) ==
             "((2.0581498, -0.6801169, 0.3778564, 1.0690141), (-0.07970169, 0.18988162, 1.2367828, -3.3141007), "
             "(-0.1329155, 1.1667843, -0.6838074, -0.08027053), (0.25, -1.6, 2.4, 0.9))");
    PX_CHECK(pxToString(glm::translate(Mf, pxVec3f(0.3f, -1.7f, 2.9f))[3]) == "(-5.2700005, 2.48, 1.8600001, -6.355)");
    PX_CHECK(pxToString(glm::scale(M, pxVec3(0.3, -1.7, 2.9))) ==
             "((0.03, 0.21, -0.39, 0.66), (-3.23, 0.68, -1.02, -0.085), (-2.32, 3.19, 0.87, -7.83), "
             "(0.25, -1.6, 2.4, 0.9))");

    // The camera and the projections are OpenGL's: right-handed, depth in [-1, 1] (1.13).
    PX_CHECK(pxToString(glm::lookAt(pxVec3(0, 0, 5), pxVec3(0), pxVec3(0, 1, 0))) ==
             "((1, 0, -0, 0), (-0, 1, -0, 0), (0, 0, 1, 0), (-0, -0, -5, 1))");
    PX_CHECK(pxToString(glm::lookAt(pxVec3f(1, 2, 3), pxVec3f(-0.5f, 0.25f, 0), pxVec3f(0, 1, 0))) ==
             "((0.8944272, -0.20686895, 0.39649117, 0), (0, 0.8865812, 0.46257302, 0), "
             "(-0.4472136, -0.4137379, 0.79298234, 0), (0.44721353, -0.3250798, -3.7005842, 1))");
    PX_CHECK(pxToString(glm::perspective(pi / 2, 16.0 / 9.0, 0.1, 100.0)) ==
             "((0.5625000000000001, 0, 0, 0), (0, 1.0000000000000002, 0, 0), "
             "(0, 0, -1.002002002002002, -1), (0, 0, -0.20020020020020018, 0))");
    PX_CHECK(pxToString(glm::perspective(pif / 2, 16.0f / 9.0f, 0.1f, 100.0f)) ==
             "((0.5625, 0, 0, 0), (0, 1, 0, 0), (0, 0, -1.002002, -1), (0, 0, -0.2002002, 0))");
    PX_CHECK(pxToString(glm::ortho(-2.5f, 3.0f, -1.5f, 2.0f, 0.1f, 100.0f)) ==
             "((0.36363637, 0, 0, 0), (0, 0.5714286, 0, 0), (0, 0, -0.02002002, 0), "
             "(-0.09090909, -0.14285715, -1.002002, 1))");

    // The 2D projection turns Y over: the top left corner of the window goes to (-1, 1).
    pxMat4 ortho2D = pxOrtho2D(800.0, 600.0);
    PX_CHECK(ortho2D == glm::ortho(0.0, 800.0, 600.0, 0.0));
    PX_CHECK(ortho2D * pxVec4(0, 0, 0, 1) == pxVec4(-1, 1, 0, 1));
    PX_CHECK(ortho2D * pxVec4(800, 600, 0, 1) == pxVec4(1, -1, 0, 1));
    PX_CHECK(ortho2D * pxVec4(400, 300, 0.5, 1) == pxVec4(0, 0, -0.5, 1));
    PX_CHECK(pxOrtho2D(800.0f, 600.0f) * pxVec4f(800, 0, 0, 1) == pxVec4f(1, 1, 0, 1));

    // The 2D transforms of a mat3 (6.6), GLM's, with the point as (x, y, 1). In T * R * S * p, the S
    // comes first; with Y down, a positive angle turns clockwise on the screen, as in 3D.
    PX_CHECK(glm::translate(pxMat3(1.0), pxVec2(3, 4)) * pxVec3(1, 1, 1) == pxVec3(4, 5, 1));
    PX_CHECK(glm::scale(pxMat3(1.0), pxVec2(2, 3)) * pxVec3(1, 1, 1) == pxVec3(2, 3, 1));
    PX_CHECK(pxToString(glm::rotate(pxMat3(1.0), pi / 2) * pxVec3(1, 0, 1)) ==
             "(0.00000000000000006123233995736766, 1, 1)");
    pxMat3 trs = glm::scale(glm::rotate(glm::translate(pxMat3(1.0), pxVec2(10, 20)), pi / 2), pxVec2(2, 3));
    PX_CHECK(trs * pxVec3(1, 1, 1) == pxVec3(7, 22, 1));
    // A point goes through as (x, y, 1), so the translation applies (8.1).
    PX_CHECK(trs * pxPoint(1, 1) == pxPoint(7, 22));
    PX_CHECK(glm::translate(pxMat3(1.0), pxVec2(3, 4)) * pxPoint(1, 1) == pxPoint(4, 5));
    PX_CHECK(glm::translate(pxMat3f(1.0f), pxVec2f(3, 4)) * pxPointf(1, 1) == pxPointf(4, 5));
    // The same bits as the translation column of glm::translate(N, (0.3, -1.7)), and as C#.
    PX_CHECK((N * pxPoint(0.3, -1.7)).ToString() == "(-4, 1.9900000000000002)");
    PX_CHECK((pxMat3f(N) * pxPointf(0.3f, -1.7f)).ToString() == "(-4, 1.99)");
    // GLM's shearX(m, k) adds k * x to y, and shearY(m, k) adds k * y to x.
    PX_CHECK(glm::shearX(pxMat3(1.0), 0.5) * pxVec3(2, 3, 1) == pxVec3(2, 4, 1));
    PX_CHECK(glm::shearY(pxMat3(1.0), 0.5) * pxVec3(2, 3, 1) == pxVec3(3.5, 3, 1));
    PX_CHECK(pxToString(glm::rotate(N, pi / 2)) ==
             "((1.9, -0.39999999999999997, 0.5999999999999999), (-0.0999999999999999, -0.7, 1.3), "
             "(-0.8, 1.1, 0.3))");
    PX_CHECK(pxToString(glm::rotate(Nf, pif / 2)) ==
             "((1.9, -0.40000004, 0.6000001), (-0.10000008, -0.7, 1.3), (-0.8, 1.1, 0.3))");
    PX_CHECK(pxToString(glm::translate(N, pxVec2(0.3, -1.7))[2]) == "(-4, 1.9900000000000002, -1.11)");
    PX_CHECK(pxToString(glm::translate(Nf, pxVec2f(0.3f, -1.7f))[2]) == "(-4, 1.99, -1.1100001)");
    PX_CHECK(pxToString(glm::scale(N, pxVec2(0.3, -1.7))) ==
             "((0.03, 0.21, -0.39), (-3.23, 0.68, -1.02), (-0.8, 1.1, 0.3))");
    PX_CHECK(pxToString(glm::shearX(N, 0.3)[0]) == "(0.6699999999999999, 0.58, -1.12)");
    PX_CHECK(pxToString(glm::shearY(Nf, -1.7f)[1]) == "(1.73, -1.59, 2.81)");

    // Between mat3 and mat4, and between precisions: explicit, as the vectors (6.3).
    PX_CHECK(pxMat3(M)[2] == pxVec3(-0.8, 1.1, 0.3));
    PX_CHECK(pxMat4(N)[3] == pxVec4(0, 0, 0, 1) && pxMat4(N)[0] == pxVec4(0.1, 0.7, -1.3, 0));
    static_assert(!std::is_convertible_v<pxMat4f, pxMat4> && std::is_constructible_v<pxMat4, pxMat4f>);
    static_assert(!std::is_convertible_v<pxMat4, pxMat3>);
    static_assert(Multipliable<pxMat4, double> && !Multipliable<pxMat4, int> && !Multipliable<pxMat4f, double>);
    PX_CHECK(M * 2.0 == M + M && 2.0 * M == M + M && (M + M) / 2.0 == M && -M == M * -1.0);

    // The text, column by column, as in memory.
    PX_CHECK(pxToString(pxMat3(1.0)) == "((1, 0, 0), (0, 1, 0), (0, 0, 1))");
    PX_CHECK(pxToString(pxMat4f(1.5f)) == "((1.5, 0, 0, 0), (0, 1.5, 0, 0), (0, 0, 1.5, 0), (0, 0, 0, 1.5))");
}
