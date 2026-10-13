# Generator

**Document ID:** GENERATOR-INTRODUCTION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** introduction to the generator module

**Surface:** introduction

**Boundary:** generator

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

C# has no templates, and the generic math of `INumber<T>` does not exist on the
.NET Framework, which PixieLib also targets. So each C# type that comes in
several precisions is written once, as a template, and a Roslyn source
generator copies it into one type per precision while PixieLib builds:
`PxPoint.template.cs` becomes `PxPoint` (`double`), `PxPointf` (`float`), and
`PxPointi` (`int`).

```csharp
// In a template: __S__ is the suffix and __T__ the type; #if picks the differences.
public struct PxVec2__S__
{
    private __T__ x;
#if PX_FLOATING
    // only in PxVec2 and PxVec2f
#endif
}
```

The generator does not ship: whoever uses PixieLib gets only `PixieLib.dll`.
The contract is the [technical reference](REFERENCE.md).
