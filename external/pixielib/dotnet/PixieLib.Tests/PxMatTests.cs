using System;
using System.Numerics;

namespace PixieLib.Tests;

// The same values as cpp/tests/pxMat.test.cpp, where the math is GLM's.
internal static class PxMatTests
{
    private static readonly PxMat4 M = new PxMat4(new PxVec4(0.1, 0.7, -1.3, 2.2), new PxVec4(1.9, -0.4, 0.6, 0.05),
                                                  new PxVec4(-0.8, 1.1, 0.3, -2.7), new PxVec4(0.25, -1.6, 2.4, 0.9));
    private static readonly PxMat3 N = new PxMat3(new PxVec3(0.1, 0.7, -1.3), new PxVec3(1.9, -0.4, 0.6), new PxVec3(-0.8, 1.1, 0.3));

    // The cosine of pi / 2 comes from the runtime, as in C++ it comes from the C library (docs/layout.md,
    // 3). The .NET Framework gives another one than .NET, glibc and the C runtime of MSVC, which agree.
#if NETFRAMEWORK
    private const string CosineOfHalfPi = "0.00000000000000006123031769111886";
#else
    private const string CosineOfHalfPi = "0.00000000000000006123233995736766";
#endif

    public static void Run()
    {
        var mf = (PxMat4f)M;
        var nf = (PxMat3f)N;

        // The default is zero, as any struct; in C++ a matrix starts as the identity.
        Check.That(default(PxMat4) == new PxMat4(0) && PxMat4.Identity == new PxMat4(1));
        Check.That(new PxMat4(2)[1] == new PxVec4(0, 2, 0, 0) && PxMat3f.Identity[2, 2] == 1);

        // Column-major and M * v (1.12): m[3] is the fourth column, and in T * S * v the S comes first.
        PxMat4 t = PxMat4.Translate(PxMat4.Identity, new PxVec3(1, 2, 3));
        PxMat4 s = PxMat4.Scale(PxMat4.Identity, new PxVec3(2, 2, 2));
        Check.That(t[3] == new PxVec4(1, 2, 3, 1) && t[3, 1] == 2);
        Check.That(t * s * new PxVec4(1, 1, 1, 1) == new PxVec4(3, 4, 5, 1));
        Check.That(s * t * new PxVec4(1, 1, 1, 1) == new PxVec4(4, 6, 8, 1));
        Check.That(PxMat4.Transpose(M)[0] == new PxVec4(0.1, 1.9, -0.8, 0.25));

        // The order of the sums is GLM's: M * v adds the columns in pairs, M1 * M2 and the mat3 add them
        // in sequence, and v * M takes the dot product of a vec4, which adds in pairs.
        var a = new PxMat4(new PxVec4(1e16, 0, 0, 0), new PxVec4(1, 0, 0, 0), new PxVec4(-1e16, 0, 0, 0), new PxVec4(1, 0, 0, 0));
        var one = new PxVec4(1);
        Check.That((a * one).X == 0);
        Check.That((a * new PxMat4(one, one, one, one))[0].X == 1);
        Check.That((one * PxMat4.Transpose(a)).X == 0);
        var b = new PxMat3(new PxVec3(1, 0, 0), new PxVec3(1e16, 0, 0), new PxVec3(-1e16, 0, 0));
        Check.That((b * new PxVec3(1)).X == 0 && (b * new PxMat3(new PxVec3(1), new PxVec3(1), new PxVec3(1)))[0].X == 0);

        // The same bits as the C++ tests.
        Check.Text((M * PxMat4.Transpose(M)),
                   "((4.3225, -1.9700000000000002, 1.3699999999999997, 2.7), " +
                   "(-1.9700000000000002, 4.420000000000001, -4.66, -2.8900000000000006), " +
                   "(1.3699999999999997, -4.66, 7.9, -1.4800000000000004), " +
                   "(2.7, -2.8900000000000006, -1.4800000000000004, 12.942500000000003))");
        Check.Text((mf * PxMat4f.Transpose(mf)),
                   "((4.3224998, -1.97, 1.37, 2.7), (-1.97, 4.42, -4.66, -2.8899999), " +
                   "(1.37, -4.66, 7.8999996, -1.4799998), (2.7, -2.8899999, -1.4799998, 12.942501))");
        Check.Text((M * new PxVec4(0.3, -1.7, 2.9, 0.6)), "(-5.37, 3.12, 0.8999999999999999, -6.715)");
        Check.Text((mf * new PxVec4f(0.3f, -1.7f, 2.9f, 0.6f)), "(-5.37, 3.12, 0.9000001, -6.715)");
        Check.Text((new PxVec4(0.3, -1.7, 2.9, 0.6) * M), "(-3.6100000000000003, 3.02, -2.8600000000000003, 10.295)");
        Check.Text((new PxVec4f(0.3f, -1.7f, 2.9f, 0.6f) * mf), "(-3.61, 3.02, -2.8600001, 10.295)");
        Check.Text((N * PxMat3.Transpose(N)),
                   "((4.26, -1.5700000000000003, 0.7699999999999998), (-1.5700000000000003, 1.86, " +
                   "-0.8199999999999998), (0.7699999999999998, -0.8199999999999998, 2.14))");
        Check.Text((N * new PxVec3(0.3, -1.7, 2.9)), "(-5.52, 4.08, -0.5400000000000001)");
        Check.Text((new PxVec3(0.3, -1.7, 2.9) * N), "(-4.93, 2.99, -1.2400000000000002)");

        // The inverse and the determinant.
        Check.Text(PxMat4.Inverse(M),
                   "((0.040714062010648304, 0.5413628025591696, 0.005219751539826708, -0.1139397193265029), " +
                   "(0.9509082367679297, 0.28130732405708914, 0.891142081636914, 0.33335570369707546), " +
                   "(0.46938615721891636, 0.14238736521855852, 0.5729982252844765, 0.5636958823617139), " +
                   "(0.4274976511118071, -0.02997628741443332, 0.05480739116818045, 0.23220437564314794))");
        Check.Text(PxMat4f.Inverse(mf),
                   "((0.040714066, 0.54136276, 0.0052197482, -0.1139397), " +
                   "(0.95090824, 0.2813073, 0.8911419, 0.33335567), " +
                   "(0.4693861, 0.14238736, 0.57299817, 0.5636958), " +
                   "(0.42749763, -0.029976288, 0.054807395, 0.23220432))");
        Check.Text(PxMat3.Inverse(N),
                   "((0.2504816955684008, 0.5266538214515094, 0.032113037893384724), " +
                   "(0.33718689788053946, 0.32434168272318564, 0.8124598587026333), " +
                   "(-0.5684007707129094, 0.21515735388567758, 0.43994861913937056))");
        Check.Text(PxMat3f.Inverse(nf),
                   "((0.2504817, 0.52665377, 0.032113027), (0.33718687, 0.32434165, 0.81245977), " +
                   "(-0.5684007, 0.21515734, 0.43994856))");
        Check.That(PxMat4.Determinant(M) == -13.4106 && PxMat4f.Determinant(mf) == -13.410602f);
        Check.That(PxMat3.Determinant(N) == -3.114 && PxMat3f.Determinant(nf) == -3.114f);

        // A matrix whose determinant and inverse change with the order of the last sums: GLM adds the
        // determinant in sequence and, in the inverse, in pairs.
        var p = new PxMat4(new PxVec4(-1.6, -1.6, -0.1, -3), new PxVec4(-3, -1.4, -2.9, 2.7),
                           new PxVec4(-0.2, -1.4, -2.1, -2.6), new PxVec4(1.1, -1.4, 1.8, 3));
        var pf = (PxMat4f)p;
        Check.That(PxMat4.Determinant(p) == 121.3586 && PxMat4f.Determinant(pf) == 121.358604f);
        Check.Text(PxMat4.Inverse(p),
                   "((-0.29013189011738755, -0.14986988973175364, 0.3410883118295695, 0.14036088089348428), " +
                   "(-0.13743566586957992, -0.02498380831683952, -0.22354410812253928, -0.30868846542395845), " +
                   "(0.2603688572544508, -0.08816845283317373, -0.29416951085460774, 0.08477355539698049), " +
                   "(-0.113976265382099, 0.09619425405368882, -0.05288459161526252, 0.08694892656968686))");
        Check.Text(PxMat4f.Inverse(pf),
                   "((-0.29013187, -0.1498699, 0.34108835, 0.14036086), " +
                   "(-0.13743566, -0.024983808, -0.22354412, -0.30868843), " +
                   "(0.26036882, -0.08816845, -0.29416952, 0.08477355), " +
                   "(-0.11397627, 0.09619425, -0.052884597, 0.08694892))");

        // The transforms. With Y down, as the 2D of PixieLib (1.13), a positive angle turns clockwise on
        // the screen: the x axis goes to (0, 1), which points down.
        Check.Text((PxMat4.Rotate(PxMat4.Identity, Math.PI / 2, new PxVec3(0, 0, 1)) * new PxVec4(1, 0, 0, 0)),
                   $"({CosineOfHalfPi}, 1, 0, 0)");
        Check.Text(PxMat4.Rotate(M, Math.PI / 2, new PxVec3(1, 2, 3)),
                   "((2.058149923103555, -0.6801167939308144, 0.3778563474377664, 1.0690141783282412), " +
                   "(-0.07970165181795241, 0.1898816152304328, 1.2367829303178968, -3.3141009783569753), " +
                   "(-0.13291553982255017, 1.1667845211566497, -0.6838074026911867, -0.08027074053809691), " +
                   "(0.25, -1.6, 2.4, 0.9))");
        Check.Text(PxMat4f.Rotate(mf, (float)Math.PI / 2, new PxVec3f(1, 2, 3)),
                   "((2.0581498, -0.6801169, 0.3778564, 1.0690141), (-0.07970169, 0.18988162, 1.2367828, -3.3141007), " +
                   "(-0.1329155, 1.1667843, -0.6838074, -0.08027053), (0.25, -1.6, 2.4, 0.9))");
        Check.Text(PxMat4f.Translate(mf, new PxVec3f(0.3f, -1.7f, 2.9f))[3], "(-5.2700005, 2.48, 1.8600001, -6.355)");
        Check.Text(PxMat4.Scale(M, new PxVec3(0.3, -1.7, 2.9)),
                   "((0.03, 0.21, -0.39, 0.66), (-3.23, 0.68, -1.02, -0.085), (-2.32, 3.19, 0.87, -7.83), " +
                   "(0.25, -1.6, 2.4, 0.9))");

        // The camera and the projections are OpenGL's: right-handed, depth in [-1, 1] (1.13).
        Check.Text(PxMat4.LookAt(new PxVec3(0, 0, 5), PxVec3.Zero, PxVec3.UnitY),
                   "((1, 0, -0, 0), (-0, 1, -0, 0), (0, 0, 1, 0), (-0, -0, -5, 1))");
        Check.Text(PxMat4f.LookAt(new PxVec3f(1, 2, 3), new PxVec3f(-0.5f, 0.25f, 0), PxVec3f.UnitY),
                   "((0.8944272, -0.20686895, 0.39649117, 0), (0, 0.8865812, 0.46257302, 0), " +
                   "(-0.4472136, -0.4137379, 0.79298234, 0), (0.44721353, -0.3250798, -3.7005842, 1))");
        Check.Text(PxMat4.Perspective(Math.PI / 2, 16.0 / 9.0, 0.1, 100.0),
                   "((0.5625000000000001, 0, 0, 0), (0, 1.0000000000000002, 0, 0), " +
                   "(0, 0, -1.002002002002002, -1), (0, 0, -0.20020020020020018, 0))");
        Check.Text(PxMat4f.Perspective((float)Math.PI / 2, 16.0f / 9.0f, 0.1f, 100.0f),
                   "((0.5625, 0, 0, 0), (0, 1, 0, 0), (0, 0, -1.002002, -1), (0, 0, -0.2002002, 0))");
        Check.Text(PxMat4f.Ortho(-2.5f, 3.0f, -1.5f, 2.0f, 0.1f, 100.0f),
                   "((0.36363637, 0, 0, 0), (0, 0.5714286, 0, 0), (0, 0, -0.02002002, 0), " +
                   "(-0.09090909, -0.14285715, -1.002002, 1))");

        // The 2D projection turns Y over: the top left corner of the window goes to (-1, 1).
        PxMat4 ortho2D = PxMat4.Ortho2D(800, 600);
        Check.That(ortho2D == PxMat4.Ortho(0, 800, 600, 0));
        Check.That(ortho2D * new PxVec4(0, 0, 0, 1) == new PxVec4(-1, 1, 0, 1));
        Check.That(ortho2D * new PxVec4(800, 600, 0, 1) == new PxVec4(1, -1, 0, 1));
        Check.That(ortho2D * new PxVec4(400, 300, 0.5, 1) == new PxVec4(0, 0, -0.5, 1));
        Check.That(PxMat4f.Ortho2D(800, 600) * new PxVec4f(800, 0, 0, 1) == new PxVec4f(1, 1, 0, 1));

        // The 2D transforms of a mat3 (6.6), with the point as (x, y, 1). In T * R * S * p, the S comes
        // first; with Y down, a positive angle turns clockwise on the screen, as in 3D.
        Check.That(PxMat3.Translate(PxMat3.Identity, new PxVec2(3, 4)) * new PxVec3(1, 1, 1) == new PxVec3(4, 5, 1));
        Check.That(PxMat3.Scale(PxMat3.Identity, new PxVec2(2, 3)) * new PxVec3(1, 1, 1) == new PxVec3(2, 3, 1));
        Check.Text((PxMat3.Rotate(PxMat3.Identity, Math.PI / 2) * new PxVec3(1, 0, 1)),
                   $"({CosineOfHalfPi}, 1, 1)");
        PxMat3 trs = PxMat3.Scale(PxMat3.Rotate(PxMat3.Translate(PxMat3.Identity, new PxVec2(10, 20)), Math.PI / 2), new PxVec2(2, 3));
        Check.That(trs * new PxVec3(1, 1, 1) == new PxVec3(7, 22, 1));
        Check.That(trs * new PxVec3((PxVec2)new PxPoint(1, 1), 1) == new PxVec3(7, 22, 1));
        // A point goes through as (x, y, 1), so the translation applies (8.1).
        Check.That(trs * new PxPoint(1, 1) == new PxPoint(7, 22));
        Check.That(PxMat3.Translate(PxMat3.Identity, new PxVec2(3, 4)) * new PxPoint(1, 1) == new PxPoint(4, 5));
        Check.That(PxMat3f.Translate(PxMat3f.Identity, new PxVec2f(3, 4)) * new PxPointf(1, 1) == new PxPointf(4, 5));
        // The same bits as the translation column of Translate(N, (0.3, -1.7)), and as C++.
        Check.Text(N * new PxPoint(0.3, -1.7), "(-4, 1.9900000000000002)");
        Check.Text((PxMat3f)N * new PxPointf(0.3f, -1.7f), "(-4, 1.99)");
        // GLM's ShearX(m, k) adds k * x to y, and ShearY(m, k) adds k * y to x.
        Check.That(PxMat3.ShearX(PxMat3.Identity, 0.5) * new PxVec3(2, 3, 1) == new PxVec3(2, 4, 1));
        Check.That(PxMat3.ShearY(PxMat3.Identity, 0.5) * new PxVec3(2, 3, 1) == new PxVec3(3.5, 3, 1));
        Check.Text(PxMat3.Rotate(N, Math.PI / 2),
                   "((1.9, -0.39999999999999997, 0.5999999999999999), (-0.0999999999999999, -0.7, 1.3), " +
                   "(-0.8, 1.1, 0.3))");
        Check.Text(PxMat3f.Rotate(nf, (float)Math.PI / 2),
                   "((1.9, -0.40000004, 0.6000001), (-0.10000008, -0.7, 1.3), (-0.8, 1.1, 0.3))");
        Check.Text(PxMat3.Translate(N, new PxVec2(0.3, -1.7))[2], "(-4, 1.9900000000000002, -1.11)");
        Check.Text(PxMat3f.Translate(nf, new PxVec2f(0.3f, -1.7f))[2], "(-4, 1.99, -1.1100001)");
        Check.Text(PxMat3.Scale(N, new PxVec2(0.3, -1.7)),
                   "((0.03, 0.21, -0.39), (-3.23, 0.68, -1.02), (-0.8, 1.1, 0.3))");
        Check.Text(PxMat3.ShearX(N, 0.3)[0], "(0.6699999999999999, 0.58, -1.12)");
        Check.Text(PxMat3f.ShearY(nf, -1.7f)[1], "(1.73, -1.59, 2.81)");

        // Between mat3 and mat4, and between precisions.
        Check.That(new PxMat3(M)[2] == new PxVec3(-0.8, 1.1, 0.3));
        Check.That(new PxMat4(N)[3] == PxVec4.UnitW && new PxMat4(N)[0] == new PxVec4(0.1, 0.7, -1.3, 0));
        PxMat4 widened = mf;
        Check.That(widened[0, 0] == 0.1f && (PxMat4f)widened == mf);
        Check.That(M * 2 == M + M && 2 * M == M + M && (M + M) / 2 == M && -M == M * -1);

        // The elements by column and row, as m[c][r] in GLM.
        PxMat4 e = PxMat4.Identity;
        e[3, 1] = 7;
        e[0] = new PxVec4(1, 2, 3, 4);
        Check.That(e[3] == new PxVec4(0, 7, 0, 1) && e[0, 2] == 3 && e[1, 1] == 1);
        PxMat3f e3 = PxMat3f.Identity;
        e3[2, 0] = 5;
        Check.That(e3[2] == new PxVec3f(5, 0, 1));
        Check.Throws<ArgumentOutOfRangeException>(() => _ = e[4]);
        Check.Throws<ArgumentOutOfRangeException>(() => _ = e[0, 4]);
        Check.Throws<ArgumentOutOfRangeException>(() => _ = e3[3]);

        // With System.Numerics, the same bytes: Vector4.Transform(v, m) is m * v, and a product of
        // Matrix4x4 goes in the other order.
        Matrix4x4 sn = mf;
        Check.That(sn.M12 == mf[0, 1] && sn.M41 == mf[3, 0] && (PxMat4f)sn == mf);
        var exact = new PxMat4f(new PxVec4f(1, 2, 3, 4), new PxVec4f(5, 6, 7, 8), new PxVec4f(9, 10, 11, 12), new PxVec4f(13, 14, 15, 16));
        var vf = new PxVec4f(1, -2, 3, 1);
        Check.That((PxVec4f)Vector4.Transform(vf, exact) == exact * vf && exact * vf == new PxVec4f(31, 34, 37, 40));
        Matrix4x4 product = (Matrix4x4)PxMat4f.Translate(PxMat4f.Identity, new PxVec3f(1, 2, 3)) * (Matrix4x4)PxMat4f.Scale(PxMat4f.Identity, new PxVec3f(2, 2, 2));
        Check.That((PxMat4f)product * new PxVec4f(1, 1, 1, 1) == new PxVec4f(4, 6, 8, 1));

        // Equals compares each element with its own Equals, so a NaN equals itself (5.5).
        var nan = new PxMat3(double.NaN);
        var sameNan = nan;
        Check.That(nan.Equals(sameNan) && !(nan == sameNan) && nan.GetHashCode() == sameNan.GetHashCode());
        var nan4 = new PxMat4f(float.NaN);
        var sameNan4 = nan4;
        Check.That(nan4.Equals(sameNan4) && !(nan4 == sameNan4) && nan4.GetHashCode() == sameNan4.GetHashCode());
        Check.That(new PxMat4(0.0).Equals(new PxMat4(-0.0)) && new PxMat4(0.0).GetHashCode() == new PxMat4(-0.0).GetHashCode());

        // The text, column by column, as in memory.
        Check.Text(PxMat3.Identity, "((1, 0, 0), (0, 1, 0), (0, 0, 1))");
        Check.Text(new PxMat4f(1.5f), "((1.5, 0, 0, 0), (0, 1.5, 0, 0), (0, 0, 1.5, 0), (0, 0, 0, 1.5))");
    }
}
