// PX_PRECISIONS: double float
using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace PixieLib;

// A 4x4 matrix in __T__, made of four columns. Same layout as pxMat4 in C++, which is the mat4 of GLM
// (docs/layout.md): column-major, and a vector is a column, multiplied on the right, M * v (1.12). In
// A * B * v, the B is applied first. The functions have the names of GLM's in PascalCase (Translate is
// glm::translate) and give the same bits: the formulas are GLM's, in the same order (docs/notas.md,
// 4.14). The projections are OpenGL's, right-handed with the depth in [-1, 1] (1.13), and not those
// of System.Numerics, which are for a depth in [0, 1].
//
// The default is zero, as for any C# struct. A matrix of C++ starts as the identity (6.3), which here
// is Identity, or new PxMat4(1).
[StructLayout(LayoutKind.Sequential)]
public struct PxMat4__S__ : IEquatable<PxMat4__S__>
{
    public static readonly PxMat4__S__ Identity = new PxMat4__S__(1);

    private PxVec4__S__ c0;
    private PxVec4__S__ c1;
    private PxVec4__S__ c2;
    private PxVec4__S__ c3;

    // The diagonal, the rest zero, as GLSL's mat4(1).
    public PxMat4__S__(__T__ diagonal)
        : this(new PxVec4__S__(diagonal, 0, 0, 0), new PxVec4__S__(0, diagonal, 0, 0),
               new PxVec4__S__(0, 0, diagonal, 0), new PxVec4__S__(0, 0, 0, diagonal))
    {
    }

    public PxMat4__S__(PxVec4__S__ c0, PxVec4__S__ c1, PxVec4__S__ c2, PxVec4__S__ c3)
    {
        this.c0 = c0;
        this.c1 = c1;
        this.c2 = c2;
        this.c3 = c3;
    }

    // A mat3 in the top left corner and the rest of the identity, as GLM's mat4(mat3).
    public PxMat4__S__(PxMat3__S__ m)
        : this(new PxVec4__S__(m[0], 0), new PxVec4__S__(m[1], 0), new PxVec4__S__(m[2], 0), PxVec4__S__.UnitW)
    {
    }

    // The column, as m[c] in GLM.
    public PxVec4__S__ this[int column]
    {
        readonly get => column switch
        {
            0 => c0,
            1 => c1,
            2 => c2,
            3 => c3,
            _ => throw new ArgumentOutOfRangeException(nameof(column)),
        };
        set
        {
            switch (column)
            {
                case 0: c0 = value; break;
                case 1: c1 = value; break;
                case 2: c2 = value; break;
                case 3: c3 = value; break;
                default: throw new ArgumentOutOfRangeException(nameof(column));
            }
        }
    }

    // The element in a column and a row, as m[c][r] in GLM. Matrix4x4 takes the row first.
    public __T__ this[int column, int row]
    {
        readonly get
        {
            PxVec4__S__ c = this[column];
            return row switch
            {
                0 => c.X,
                1 => c.Y,
                2 => c.Z,
                3 => c.W,
                _ => throw new ArgumentOutOfRangeException(nameof(row)),
            };
        }
        set
        {
            PxVec4__S__ c = this[column];
            switch (row)
            {
                case 0: c.X = value; break;
                case 1: c.Y = value; break;
                case 2: c.Z = value; break;
                case 3: c.W = value; break;
                default: throw new ArgumentOutOfRangeException(nameof(row));
            }
            this[column] = c;
        }
    }

    public static PxMat4__S__ operator +(PxMat4__S__ a, PxMat4__S__ b) => new PxMat4__S__(a.c0 + b.c0, a.c1 + b.c1, a.c2 + b.c2, a.c3 + b.c3);
    public static PxMat4__S__ operator -(PxMat4__S__ a, PxMat4__S__ b) => new PxMat4__S__(a.c0 - b.c0, a.c1 - b.c1, a.c2 - b.c2, a.c3 - b.c3);
    public static PxMat4__S__ operator +(PxMat4__S__ m) => m;
    public static PxMat4__S__ operator -(PxMat4__S__ m) => new PxMat4__S__(-m.c0, -m.c1, -m.c2, -m.c3);
    public static PxMat4__S__ operator *(PxMat4__S__ m, __T__ k) => new PxMat4__S__(m.c0 * k, m.c1 * k, m.c2 * k, m.c3 * k);
    public static PxMat4__S__ operator *(__T__ k, PxMat4__S__ m) => m * k;
    public static PxMat4__S__ operator /(PxMat4__S__ m, __T__ k) => new PxMat4__S__(m.c0 / k, m.c1 / k, m.c2 / k, m.c3 / k);

    // M * v adds the columns in pairs, as GLM does.
    public static PxVec4__S__ operator *(PxMat4__S__ m, PxVec4__S__ v) =>
        (m.c0 * v.X + m.c1 * v.Y) + (m.c2 * v.Z + m.c3 * v.W);

    // v * M, the vector as a row: the dot product with each column, as GLM does.
    public static PxVec4__S__ operator *(PxVec4__S__ v, PxMat4__S__ m) =>
        new PxVec4__S__(PxVec4__S__.Dot(m.c0, v), PxVec4__S__.Dot(m.c1, v), PxVec4__S__.Dot(m.c2, v), PxVec4__S__.Dot(m.c3, v));

    // The product of two matrices adds the columns in sequence, unlike M * v: it is GLM's order.
    public static PxMat4__S__ operator *(PxMat4__S__ a, PxMat4__S__ b) =>
        new PxMat4__S__(Combine(a, b.c0), Combine(a, b.c1), Combine(a, b.c2), Combine(a, b.c3));

    private static PxVec4__S__ Combine(PxMat4__S__ a, PxVec4__S__ b) => a.c0 * b.X + a.c1 * b.Y + a.c2 * b.Z + a.c3 * b.W;

    public static PxMat4__S__ Transpose(PxMat4__S__ m) => new PxMat4__S__(
        new PxVec4__S__(m.c0.X, m.c1.X, m.c2.X, m.c3.X),
        new PxVec4__S__(m.c0.Y, m.c1.Y, m.c2.Y, m.c3.Y),
        new PxVec4__S__(m.c0.Z, m.c1.Z, m.c2.Z, m.c3.Z),
        new PxVec4__S__(m.c0.W, m.c1.W, m.c2.W, m.c3.W));

    // glm::determinant. In the names, m12 is m[1][2]: column 1, row 2.
    public static __T__ Determinant(PxMat4__S__ m)
    {
        __T__ m00 = m.c0.X, m01 = m.c0.Y, m02 = m.c0.Z, m03 = m.c0.W;
        __T__ m10 = m.c1.X, m11 = m.c1.Y, m12 = m.c1.Z, m13 = m.c1.W;
        __T__ m20 = m.c2.X, m21 = m.c2.Y, m22 = m.c2.Z, m23 = m.c2.W;
        __T__ m30 = m.c3.X, m31 = m.c3.Y, m32 = m.c3.Z, m33 = m.c3.W;

        __T__ subFactor00 = m22 * m33 - m32 * m23;
        __T__ subFactor01 = m21 * m33 - m31 * m23;
        __T__ subFactor02 = m21 * m32 - m31 * m22;
        __T__ subFactor03 = m20 * m33 - m30 * m23;
        __T__ subFactor04 = m20 * m32 - m30 * m22;
        __T__ subFactor05 = m20 * m31 - m30 * m21;

        __T__ detCof0 = +(m11 * subFactor00 - m12 * subFactor01 + m13 * subFactor02);
        __T__ detCof1 = -(m10 * subFactor00 - m12 * subFactor03 + m13 * subFactor04);
        __T__ detCof2 = +(m10 * subFactor01 - m11 * subFactor03 + m13 * subFactor05);
        __T__ detCof3 = -(m10 * subFactor02 - m11 * subFactor04 + m12 * subFactor05);

        return m00 * detCof0 + m01 * detCof1 + m02 * detCof2 + m03 * detCof3;
    }

    // glm::inverse, which in C++ is pxInverse. A matrix with no inverse gives infinities and NaN.
    public static PxMat4__S__ Inverse(PxMat4__S__ m)
    {
        __T__ m00 = m.c0.X, m01 = m.c0.Y, m02 = m.c0.Z, m03 = m.c0.W;
        __T__ m10 = m.c1.X, m11 = m.c1.Y, m12 = m.c1.Z, m13 = m.c1.W;
        __T__ m20 = m.c2.X, m21 = m.c2.Y, m22 = m.c2.Z, m23 = m.c2.W;
        __T__ m30 = m.c3.X, m31 = m.c3.Y, m32 = m.c3.Z, m33 = m.c3.W;

        __T__ coef00 = m22 * m33 - m32 * m23;
        __T__ coef02 = m12 * m33 - m32 * m13;
        __T__ coef03 = m12 * m23 - m22 * m13;

        __T__ coef04 = m21 * m33 - m31 * m23;
        __T__ coef06 = m11 * m33 - m31 * m13;
        __T__ coef07 = m11 * m23 - m21 * m13;

        __T__ coef08 = m21 * m32 - m31 * m22;
        __T__ coef10 = m11 * m32 - m31 * m12;
        __T__ coef11 = m11 * m22 - m21 * m12;

        __T__ coef12 = m20 * m33 - m30 * m23;
        __T__ coef14 = m10 * m33 - m30 * m13;
        __T__ coef15 = m10 * m23 - m20 * m13;

        __T__ coef16 = m20 * m32 - m30 * m22;
        __T__ coef18 = m10 * m32 - m30 * m12;
        __T__ coef19 = m10 * m22 - m20 * m12;

        __T__ coef20 = m20 * m31 - m30 * m21;
        __T__ coef22 = m10 * m31 - m30 * m11;
        __T__ coef23 = m10 * m21 - m20 * m11;

        var fac0 = new PxVec4__S__(coef00, coef00, coef02, coef03);
        var fac1 = new PxVec4__S__(coef04, coef04, coef06, coef07);
        var fac2 = new PxVec4__S__(coef08, coef08, coef10, coef11);
        var fac3 = new PxVec4__S__(coef12, coef12, coef14, coef15);
        var fac4 = new PxVec4__S__(coef16, coef16, coef18, coef19);
        var fac5 = new PxVec4__S__(coef20, coef20, coef22, coef23);

        var vec0 = new PxVec4__S__(m10, m00, m00, m00);
        var vec1 = new PxVec4__S__(m11, m01, m01, m01);
        var vec2 = new PxVec4__S__(m12, m02, m02, m02);
        var vec3 = new PxVec4__S__(m13, m03, m03, m03);

        PxVec4__S__ inv0 = vec1 * fac0 - vec2 * fac1 + vec3 * fac2;
        PxVec4__S__ inv1 = vec0 * fac0 - vec2 * fac3 + vec3 * fac4;
        PxVec4__S__ inv2 = vec0 * fac1 - vec1 * fac3 + vec3 * fac5;
        PxVec4__S__ inv3 = vec0 * fac2 - vec1 * fac4 + vec2 * fac5;

        var signA = new PxVec4__S__(+1, -1, +1, -1);
        var signB = new PxVec4__S__(-1, +1, -1, +1);
        var inverse = new PxMat4__S__(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

        var row0 = new PxVec4__S__(inverse.c0.X, inverse.c1.X, inverse.c2.X, inverse.c3.X);
        PxVec4__S__ dot0 = m.c0 * row0;
        __T__ dot1 = (dot0.X + dot0.Y) + (dot0.Z + dot0.W);

        __T__ oneOverDeterminant = 1 / dot1;
        return inverse * oneOverDeterminant;
    }

    // glm::translate: m times a translation, so the translation is applied before m.
    public static PxMat4__S__ Translate(PxMat4__S__ m, PxVec3__S__ v)
    {
        PxMat4__S__ result = m;
        result.c3 = m.c0 * v.X + m.c1 * v.Y + m.c2 * v.Z + m.c3;
        return result;
    }

    // glm::rotate: m times a rotation of angle radians around the axis, which is normalized. Looking
    // from the tip of the axis, a positive angle turns counterclockwise; on the 2D of PixieLib, with
    // Y down, a positive angle around Z turns clockwise on the screen.
    public static PxMat4__S__ Rotate(PxMat4__S__ m, __T__ angle, PxVec3__S__ axis)
    {
        __T__ c = PxMath.Cos(angle);
        __T__ s = PxMath.Sin(angle);

        PxVec3__S__ a = PxVec3__S__.Normalize(axis);
        PxVec3__S__ temp = (1 - c) * a;

        __T__ r00 = c + temp.X * a.X;
        __T__ r01 = temp.X * a.Y + s * a.Z;
        __T__ r02 = temp.X * a.Z - s * a.Y;

        __T__ r10 = temp.Y * a.X - s * a.Z;
        __T__ r11 = c + temp.Y * a.Y;
        __T__ r12 = temp.Y * a.Z + s * a.X;

        __T__ r20 = temp.Z * a.X + s * a.Y;
        __T__ r21 = temp.Z * a.Y - s * a.X;
        __T__ r22 = c + temp.Z * a.Z;

        return new PxMat4__S__(
            m.c0 * r00 + m.c1 * r01 + m.c2 * r02,
            m.c0 * r10 + m.c1 * r11 + m.c2 * r12,
            m.c0 * r20 + m.c1 * r21 + m.c2 * r22,
            m.c3);
    }

    // glm::scale: m times a scale.
    public static PxMat4__S__ Scale(PxMat4__S__ m, PxVec3__S__ v) => new PxMat4__S__(m.c0 * v.X, m.c1 * v.Y, m.c2 * v.Z, m.c3);

    // glm::lookAt, right-handed: the camera at eye looks at center, and up says where its top is.
    public static PxMat4__S__ LookAt(PxVec3__S__ eye, PxVec3__S__ center, PxVec3__S__ up)
    {
        PxVec3__S__ f = PxVec3__S__.Normalize(center - eye);
        PxVec3__S__ s = PxVec3__S__.Normalize(PxVec3__S__.Cross(f, up));
        PxVec3__S__ u = PxVec3__S__.Cross(s, f);

        return new PxMat4__S__(
            new PxVec4__S__(s.X, u.X, -f.X, 0),
            new PxVec4__S__(s.Y, u.Y, -f.Y, 0),
            new PxVec4__S__(s.Z, u.Z, -f.Z, 0),
            new PxVec4__S__(-PxVec3__S__.Dot(s, eye), -PxVec3__S__.Dot(u, eye), PxVec3__S__.Dot(f, eye), 1));
    }

    // glm::ortho with no near and far planes: z comes out negated.
    public static PxMat4__S__ Ortho(__T__ left, __T__ right, __T__ bottom, __T__ top)
    {
        PxMat4__S__ result = Identity;
        result.c0.X = 2 / (right - left);
        result.c1.Y = 2 / (top - bottom);
        result.c2.Z = -1;
        result.c3.X = -(right + left) / (right - left);
        result.c3.Y = -(top + bottom) / (top - bottom);
        return result;
    }

    // glm::ortho, right-handed, with the depth from near to far going to [-1, 1].
    public static PxMat4__S__ Ortho(__T__ left, __T__ right, __T__ bottom, __T__ top, __T__ zNear, __T__ zFar)
    {
        PxMat4__S__ result = Identity;
        result.c0.X = 2 / (right - left);
        result.c1.Y = 2 / (top - bottom);
        result.c2.Z = -2 / (zFar - zNear);
        result.c3.X = -(right + left) / (right - left);
        result.c3.Y = -(top + bottom) / (top - bottom);
        result.c3.Z = -(zFar + zNear) / (zFar - zNear);
        return result;
    }

    // glm::perspective, right-handed, with the depth in [-1, 1]; fovy is the vertical angle, in radians.
    public static PxMat4__S__ Perspective(__T__ fovy, __T__ aspect, __T__ zNear, __T__ zFar)
    {
        __T__ tanHalfFovy = PxMath.Tan(fovy / 2);

        PxMat4__S__ result = default;
        result.c0.X = 1 / (aspect * tanHalfFovy);
        result.c1.Y = 1 / tanHalfFovy;
        result.c2.Z = -(zFar + zNear) / (zFar - zNear);
        result.c2.W = -1;
        result.c3.Z = -(2 * zFar * zNear) / (zFar - zNear);
        return result;
    }

    // pxOrtho2D: the projection of the 2D, with the origin at the top left and Y down (1.13). (0, 0)
    // goes to the top left corner of the screen, (-1, 1), and (width, height) to the bottom right.
    public static PxMat4__S__ Ortho2D(__T__ width, __T__ height) => Ortho(0, width, height, 0);

#if PX_FLOAT
    // With System.Numerics, both ways: the same bytes, with nothing lost. Matrix4x4 multiplies a row on
    // the left, so the same bytes are the same transformation: Vector4.Transform(v, m) is m * v. A
    // product of Matrix4x4 goes in the other order, (Matrix4x4)(a * b) == (Matrix4x4)b * (Matrix4x4)a.
    public static implicit operator Matrix4x4(PxMat4f m) => new Matrix4x4(
        m.c0.X, m.c0.Y, m.c0.Z, m.c0.W,
        m.c1.X, m.c1.Y, m.c1.Z, m.c1.W,
        m.c2.X, m.c2.Y, m.c2.Z, m.c2.W,
        m.c3.X, m.c3.Y, m.c3.Z, m.c3.W);

    public static implicit operator PxMat4f(Matrix4x4 m) => new PxMat4f(
        new PxVec4f(m.M11, m.M12, m.M13, m.M14),
        new PxVec4f(m.M21, m.M22, m.M23, m.M24),
        new PxVec4f(m.M31, m.M32, m.M33, m.M34),
        new PxVec4f(m.M41, m.M42, m.M43, m.M44));
#endif

    // Between precisions: implicit to double, which loses nothing; explicit to float. In C++, through
    // GLM, both are explicit.
#if PX_DOUBLE
    public static implicit operator PxMat4(PxMat4f m) => new PxMat4(m[0], m[1], m[2], m[3]);
    public static explicit operator PxMat4f(PxMat4 m) => new PxMat4f((PxVec4f)m.c0, (PxVec4f)m.c1, (PxVec4f)m.c2, (PxVec4f)m.c3);
#endif

    public static bool operator ==(PxMat4__S__ a, PxMat4__S__ b) => a.c0 == b.c0 && a.c1 == b.c1 && a.c2 == b.c2 && a.c3 == b.c3;
    public static bool operator !=(PxMat4__S__ a, PxMat4__S__ b) => !(a == b);

    // Equals compares each element with its own Equals, so a NaN equals itself; == follows IEEE (5.5).
    public readonly bool Equals(PxMat4__S__ other) =>
        c0.Equals(other.c0) && c1.Equals(other.c1) && c2.Equals(other.c2) && c3.Equals(other.c3);
    public override readonly bool Equals(object? obj) => obj is PxMat4__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(c0.GetHashCode(), c1.GetHashCode(), c2.GetHashCode(), c3.GetHashCode());

    // Column by column, as in memory: "((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))".
    public override readonly string ToString() => PxText.Tuple(c0.ToString(), c1.ToString(), c2.ToString(), c3.ToString());
}
