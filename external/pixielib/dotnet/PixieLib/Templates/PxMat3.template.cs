// PX_PRECISIONS: double float
using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A 3x3 matrix in __T__, made of three columns. Same layout as pxMat3 in C++, which is the mat3 of GLM
// (docs/layout.md), with the conventions of PxMat4: column-major, M * v (1.12), GLM's names and bits
// (docs/notas.md, 4.14). It is also the matrix of the 2D transforms (6.6): m * point is (x, y, 1).
// System.Numerics has no 3x3 matrix, so there is no conversion with it.
//
// The default is zero; a matrix of C++ starts as the identity (6.3), which here is Identity.
[StructLayout(LayoutKind.Sequential)]
public struct PxMat3__S__ : IEquatable<PxMat3__S__>
{
    public static readonly PxMat3__S__ Identity = new PxMat3__S__(1);

    private PxVec3__S__ c0;
    private PxVec3__S__ c1;
    private PxVec3__S__ c2;

    // The diagonal, the rest zero, as GLSL's mat3(1).
    public PxMat3__S__(__T__ diagonal)
        : this(new PxVec3__S__(diagonal, 0, 0), new PxVec3__S__(0, diagonal, 0), new PxVec3__S__(0, 0, diagonal))
    {
    }

    public PxMat3__S__(PxVec3__S__ c0, PxVec3__S__ c1, PxVec3__S__ c2)
    {
        this.c0 = c0;
        this.c1 = c1;
        this.c2 = c2;
    }

    // The top left corner of a mat4, as GLM's mat3(mat4).
    public PxMat3__S__(PxMat4__S__ m)
        : this(Corner(m[0]), Corner(m[1]), Corner(m[2]))
    {
    }

    private static PxVec3__S__ Corner(PxVec4__S__ v) => new PxVec3__S__(v.X, v.Y, v.Z);

    // The column, as m[c] in GLM.
    public PxVec3__S__ this[int column]
    {
        readonly get => column switch
        {
            0 => c0,
            1 => c1,
            2 => c2,
            _ => throw new ArgumentOutOfRangeException(nameof(column)),
        };
        set
        {
            switch (column)
            {
                case 0: c0 = value; break;
                case 1: c1 = value; break;
                case 2: c2 = value; break;
                default: throw new ArgumentOutOfRangeException(nameof(column));
            }
        }
    }

    // The element in a column and a row, as m[c][r] in GLM.
    public __T__ this[int column, int row]
    {
        readonly get
        {
            PxVec3__S__ c = this[column];
            return row switch
            {
                0 => c.X,
                1 => c.Y,
                2 => c.Z,
                _ => throw new ArgumentOutOfRangeException(nameof(row)),
            };
        }
        set
        {
            PxVec3__S__ c = this[column];
            switch (row)
            {
                case 0: c.X = value; break;
                case 1: c.Y = value; break;
                case 2: c.Z = value; break;
                default: throw new ArgumentOutOfRangeException(nameof(row));
            }
            this[column] = c;
        }
    }

    public static PxMat3__S__ operator +(PxMat3__S__ a, PxMat3__S__ b) => new PxMat3__S__(a.c0 + b.c0, a.c1 + b.c1, a.c2 + b.c2);
    public static PxMat3__S__ operator -(PxMat3__S__ a, PxMat3__S__ b) => new PxMat3__S__(a.c0 - b.c0, a.c1 - b.c1, a.c2 - b.c2);
    public static PxMat3__S__ operator +(PxMat3__S__ m) => m;
    public static PxMat3__S__ operator -(PxMat3__S__ m) => new PxMat3__S__(-m.c0, -m.c1, -m.c2);
    public static PxMat3__S__ operator *(PxMat3__S__ m, __T__ k) => new PxMat3__S__(m.c0 * k, m.c1 * k, m.c2 * k);
    public static PxMat3__S__ operator *(__T__ k, PxMat3__S__ m) => m * k;
    public static PxMat3__S__ operator /(PxMat3__S__ m, __T__ k) => new PxMat3__S__(m.c0 / k, m.c1 / k, m.c2 / k);

    // M * v and the product of two matrices add the columns in sequence, as GLM does for the mat3.
    public static PxVec3__S__ operator *(PxMat3__S__ m, PxVec3__S__ v) => m.c0 * v.X + m.c1 * v.Y + m.c2 * v.Z;

    // v * M, the vector as a row: the dot product with each column.
    public static PxVec3__S__ operator *(PxVec3__S__ v, PxMat3__S__ m) =>
        new PxVec3__S__(PxVec3__S__.Dot(m.c0, v), PxVec3__S__.Dot(m.c1, v), PxVec3__S__.Dot(m.c2, v));

    public static PxMat3__S__ operator *(PxMat3__S__ a, PxMat3__S__ b) => new PxMat3__S__(a * b.c0, a * b.c1, a * b.c2);

    // A point through the matrix (8.1): as (x, y, 1), so the translation applies, which is what tells a
    // point from a vector. The same bits as m * vec3(x, y, 1) in GLM, as pxMat3 * pxPoint in C++. A
    // PxVec2 through it, as (x, y, 0), waits until something needs it.
    public static PxPoint__S__ operator *(PxMat3__S__ m, PxPoint__S__ p)
    {
        PxVec3__S__ v = m * new PxVec3__S__(p.X, p.Y, 1);
        return new PxPoint__S__(v.X, v.Y);
    }

    public static PxMat3__S__ Transpose(PxMat3__S__ m) => new PxMat3__S__(
        new PxVec3__S__(m.c0.X, m.c1.X, m.c2.X),
        new PxVec3__S__(m.c0.Y, m.c1.Y, m.c2.Y),
        new PxVec3__S__(m.c0.Z, m.c1.Z, m.c2.Z));

    // glm::determinant. In the names, m12 is m[1][2]: column 1, row 2.
    public static __T__ Determinant(PxMat3__S__ m)
    {
        __T__ m00 = m.c0.X, m01 = m.c0.Y, m02 = m.c0.Z;
        __T__ m10 = m.c1.X, m11 = m.c1.Y, m12 = m.c1.Z;
        __T__ m20 = m.c2.X, m21 = m.c2.Y, m22 = m.c2.Z;

        return +m00 * (m11 * m22 - m21 * m12)
               - m10 * (m01 * m22 - m21 * m02)
               + m20 * (m01 * m12 - m11 * m02);
    }

    // glm::inverse. A matrix with no inverse gives infinities and NaN.
    public static PxMat3__S__ Inverse(PxMat3__S__ m)
    {
        __T__ m00 = m.c0.X, m01 = m.c0.Y, m02 = m.c0.Z;
        __T__ m10 = m.c1.X, m11 = m.c1.Y, m12 = m.c1.Z;
        __T__ m20 = m.c2.X, m21 = m.c2.Y, m22 = m.c2.Z;

        __T__ oneOverDeterminant = 1 / Determinant(m);

        var inverse = new PxMat3__S__(
            new PxVec3__S__(+(m11 * m22 - m21 * m12), -(m01 * m22 - m21 * m02), +(m01 * m12 - m11 * m02)),
            new PxVec3__S__(-(m10 * m22 - m20 * m12), +(m00 * m22 - m20 * m02), -(m00 * m12 - m10 * m02)),
            new PxVec3__S__(+(m10 * m21 - m20 * m11), -(m00 * m21 - m20 * m01), +(m00 * m11 - m10 * m01)));
        return inverse * oneOverDeterminant;
    }

    // The 2D transforms of GLM (gtx/matrix_transform_2d), with the same names and bits. Each one is m
    // times the transform, so the transform is applied before m: in T * R * S * p, the S comes first.

    // glm::translate(m, vec2).
    public static PxMat3__S__ Translate(PxMat3__S__ m, PxVec2__S__ v)
    {
        PxMat3__S__ result = m;
        result.c2 = m.c0 * v.X + m.c1 * v.Y + m.c2;
        return result;
    }

    // glm::rotate(m, angle), in radians. With Y down, as the 2D of PixieLib (1.13), a positive angle
    // turns clockwise on the screen.
    public static PxMat3__S__ Rotate(PxMat3__S__ m, __T__ angle)
    {
        __T__ c = PxMath.Cos(angle);
        __T__ s = PxMath.Sin(angle);
        return new PxMat3__S__(m.c0 * c + m.c1 * s, m.c0 * -s + m.c1 * c, m.c2);
    }

    // glm::scale(m, vec2).
    public static PxMat3__S__ Scale(PxMat3__S__ m, PxVec2__S__ v) => new PxMat3__S__(m.c0 * v.X, m.c1 * v.Y, m.c2);

    // glm::shearX(m, k), which adds k * x to y, and glm::shearY(m, k), which adds k * y to x.
    public static PxMat3__S__ ShearX(PxMat3__S__ m, __T__ k) =>
        m * new PxMat3__S__(new PxVec3__S__(1, k, 0), new PxVec3__S__(0, 1, 0), new PxVec3__S__(0, 0, 1));

    public static PxMat3__S__ ShearY(PxMat3__S__ m, __T__ k) =>
        m * new PxMat3__S__(new PxVec3__S__(1, 0, 0), new PxVec3__S__(k, 1, 0), new PxVec3__S__(0, 0, 1));

    // Between precisions: implicit to double, which loses nothing; explicit to float. In C++, through
    // GLM, both are explicit.
#if PX_DOUBLE
    public static implicit operator PxMat3(PxMat3f m) => new PxMat3(m[0], m[1], m[2]);
    public static explicit operator PxMat3f(PxMat3 m) => new PxMat3f((PxVec3f)m.c0, (PxVec3f)m.c1, (PxVec3f)m.c2);
#endif

    public static bool operator ==(PxMat3__S__ a, PxMat3__S__ b) => a.c0 == b.c0 && a.c1 == b.c1 && a.c2 == b.c2;
    public static bool operator !=(PxMat3__S__ a, PxMat3__S__ b) => !(a == b);

    // Equals compares each element with its own Equals, so a NaN equals itself; == follows IEEE (5.5).
    public readonly bool Equals(PxMat3__S__ other) => c0.Equals(other.c0) && c1.Equals(other.c1) && c2.Equals(other.c2);
    public override readonly bool Equals(object? obj) => obj is PxMat3__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Combine(c0.GetHashCode(), c1.GetHashCode()), c2.GetHashCode());

    // Column by column, as in memory: "((1, 0, 0), (0, 1, 0), (0, 0, 1))".
    public override readonly string ToString() => PxText.Tuple(c0.ToString(), c1.ToString(), c2.ToString());
}
