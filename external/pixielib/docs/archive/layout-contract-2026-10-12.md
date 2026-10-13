# Imported: Layout Contract (pt-BR)

**Document ID:** ARCHIVE-LAYOUT-CONTRACT-20261012

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the layout of every primitive in C++ and C#, and the semantics both sides repeat, as written in Portuguese before the policy

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PXL-ARCHIVE-006

**Type:** import

**Origin:** owner decision: the documentation migration of 2026-10-12 (PXL-TODO-001)

**Author:** claude

**Reference date:** 2026-10-12

**Commits:** with this record (parent `e0186c65321fd112d389bdabd11a6e2b3d748a6d`)

**Reference commit:** `b1caee2c4d0fdf9e15b485d356994cba8be6b4fd`, the last commit that changed the document

**Original path:** `docs/layout.md`, in this repository

**Current state:** The [layout contract](../layout.md), in English, at the original path

The body below is the document as it stood at the reference commit, unchanged and in Portuguese
(pt-BR). It predates the [documentation policy](../governance/documentation-policy.md), which was
adopted on 2026-10-12; current documents are in English. The same contract continues, in English, at
its original path, where 36 source comments cite it. Current facts were extracted, in English, to
their current owners; where this record and a current owner disagree, the current owner wins.

---

# O layout dos primitivos

O contrato entre o C++ e o C# (`notas.md`, seção 1): cada primitivo tem os mesmos campos, na mesma
ordem e com os mesmos tipos nas duas linguagens, e atravessa P/Invoke sem conversão. No C++, cada
header confere o próprio layout com `static_assert` (tamanho e posição de cada campo), aplicado no
commit `0424940`; no C#, as structs são sequenciais (`[StructLayout(LayoutKind.Sequential)]`), com
os campos nesta ordem, e os testes conferem tamanho e posição pelo `Marshal` e pelo `Unsafe`,
aplicado no commit `c841ae5`.

---

## 1. Os tipos

O tamanho está em bytes, por precisão: `double` (sem sufixo), `float` (`f`), `int32_t` (`i`).

| C++ | C# | Campos, na ordem | double | float | int32 |
|---|---|---|---|---|---|
| `pxPoint` | `PxPoint` | `x`, `y` | 16 | 8 | 8 |
| `pxSize` | `PxSize` | `width`, `height` | 16 | 8 | 8 |
| `pxRect` | `PxRect` | `x`, `y`, `width`, `height` | 32 | 16 | 16 |
| `pxRegion` | `PxRegion` | `x1`, `y1`, `x2`, `y2` | 32 | 16 | 16 |
| `pxPadding` | `PxPadding` | `left`, `top`, `right`, `bottom` | 32 | 16 | 16 |
| `pxVec2` | `PxVec2` | `x`, `y` | 16 | 8 | 8 |
| `pxVec3` | `PxVec3` | `x`, `y`, `z` | 24 | 12 | 12 |
| `pxVec4` | `PxVec4` | `x`, `y`, `z`, `w` | 32 | 16 | 16 |
| `pxSegment` | `PxSegment` | `start`, `end`, dois pontos, um depois do outro | 32 | 16 | 16 |
| `pxMat3` | `PxMat3` | três colunas `pxVec3`, uma depois da outra | 72 | 36 | não tem |
| `pxMat4` | `PxMat4` | quatro colunas `pxVec4`, uma depois da outra | 128 | 64 | não tem |

Os vetores e as matrizes do C++ são os do GLM (`notas.md`, 4.13 e 4.14), nos tipos sem alinhamento
extra, os padrão. As matrizes são column-major: na memória vem a primeira coluna inteira, depois a
segunda. Os tipos de `float` do C# têm o mesmo layout que o `Vector2`, o `Vector3`, o `Vector4` e o
`Matrix4x4` do `System.Numerics`: os bytes da coluna 0 da `PxMat4f` são o `M11` a `M14` do
`Matrix4x4`.

As cores têm uma precisão só:

| C++ | C# | Campos, na ordem | Tamanho |
|---|---|---|---|
| `pxColorRgba` | `PxColorRgba` | `r`, `g`, `b`, `a`, um byte cada | 4 |
| `pxColorHsl` | `PxColorHsl` | `h`, `s`, `l` em `double`, `a` em byte | 32 |

Só a `pxColorHsl` tem enchimento: os três `double` ocupam 24 bytes, o alfa o 25º, e sete bytes
completam o tamanho até um múltiplo de 8, o alinhamento do `double`.

## 2. Regras

- Inteiro é sempre `int32_t` no C++ e `int` no C#; byte é `uint8_t` e `byte`. Nada de `long`, que
  muda de tamanho entre plataformas no C++, nem de `bool`, que não tem layout fixo no P/Invoke.
- Um tipo novo entra nas tabelas acima antes de ganhar código nas duas pontas.

## 3. A semântica que as duas pontas repetem

O layout garante que os dados atravessam; estas regras garantem que eles querem dizer o mesmo.

- `Contains`, no retângulo e na região: as bordas esquerda e de cima estão dentro, a direita e a de
  baixo não (`x >= x1 && x < x2`).
- Região e retângulo: `x2 = x + width`, `y2 = y + height`.
- Entre precisões: implícita quando não perde nada (`float` e `int32` para `double`), explícita
  quando perde. Para `int32`, trunca, satura fora do intervalo e leva NaN a 0 (`notas.md`, 4.12).
- Inteiros: a conta volta ao contrário quando passa do `int32` (`INT32_MAX + 1` é `INT32_MIN`), e a
  divisão por zero é pré-condição. As bordas do retângulo, nas operações dele, são calculadas em 64
  bits.
- Igualdade: o `==` segue o IEEE nas duas pontas (NaN é diferente de tudo); no C#, o `Equals` diz que
  NaN é igual a NaN, para o primitivo servir de chave.
- Operações do retângulo e da região: `Intersect` dá o vazio (todos os campos zero) quando os dois não
  se tocam; quem só encosta não intersecta, e quem não tem área não intersecta nada nem entra no
  `Union`. O `Contains` de um retângulo olha só as bordas.
- HSL para RGBA: a meia unidade arredonda para o par (o padrão do `Math.Round` do C#; no C++,
  `std::nearbyint`), e o valor é preso entre 0 e 255 antes.
- Texto (`ToString`): os campos na ordem, entre parênteses, separados por `, `, como em `(1.5, -2)`
  e `(120, 1, 0.5, 255)`. O ponto decimal é ponto em qualquer cultura. O número sai com os menos
  dígitos significativos que voltam ao mesmo valor, completados com zeros e sem notação científica:
  `100000`, não `1e+05`; o `float` 1e20 sai `100000000000000000000`. NaN sai `nan`, qualquer que
  seja o sinal, e os infinitos, `inf` e `-inf`. Bytes e inteiros saem como números. A HSL não tem
  hex (`notas.md`, 4.2). Conferido: o texto de 44.905 `double` e `float` aleatórios é o mesmo nas
  duas linguagens (`notas.md`, 4.11). No C#, os dígitos saem do valor exato, e não do runtime,
  porque o .NET Framework não arredonda sempre o último (`notas.md`, 4.16).
- Vetores: a matemática do C++ é a do GLM, e o C# usa as mesmas fórmulas, na mesma ordem; o `Dot`, o
  `Length`, o `Distance`, o `Cross`, o `Normalize` e o `Lerp` dão os mesmos bits em `double` e em
  `float` (`notas.md`, 4.13). Isso vale com o C++ compilado sem juntar multiplicação e soma numa
  instrução só (FMA): no GCC e no clang, com `-ffp-contract=off`, porque o clang junta por padrão, e
  com otimização até num processador sem FMA, nas contas que faz ao compilar; o MSVC não junta com o
  `/fp:precise` padrão (`notas.md`, 4.16). O produto escalar do `vec4` soma em pares, `(x + y) + (z
  + w)`, nas duas pontas, menos no GLM compilado pelo MSVC, que soma em sequência: lá, o `Dot`, o
  `Length` e o `Normalize` do `vec4`, e o `v * M` da `mat4`, podem mudar no último bit. O `Min`, o
  `Max`, o `Clamp` e o `Abs` com NaN ou -0 seguem cada biblioteca. No vetor inteiro, o overflow e a
  conversão fora do intervalo são indefinidos no C++, porque a conta é do GLM, e voltam ao contrário
  e saturam no C#.
- Matrizes: o vetor é coluna e multiplica à direita, `M * v`; em `A * B * v`, o `B` vem primeiro. O
  C# usa as fórmulas do GLM, na mesma ordem, e dá os mesmos bits em `double` e em `float`
  (`notas.md`, 4.14): o `M * v` da `mat4` soma as colunas em pares, o produto de matrizes e a `mat3`
  somam em sequência, e a inversa multiplica por 1 / determinante. O seno, o cosseno e a tangente do
  `Rotate` e do `Perspective` vêm da biblioteca C da plataforma nas duas pontas; no `net481`, o
  `float` passa pelo `double`, e o cosseno do .NET Framework pode ser outro (o de pi / 2 não é o da
  glibc nem o do MSVC; `notas.md`, 4.16). A `Matrix4x4` do `System.Numerics` tem os mesmos bytes,
  mas multiplica com o vetor à esquerda (`v * M`), então o produto de duas fica na ordem contrária,
  e as contas dela não dão os mesmos bits que as do GLM.
- Projeções e eixos: as do OpenGL, com a mão direita e a profundidade em [-1, 1]. O 2D tem a origem
  no canto superior esquerdo e o Y para baixo, e a `Ortho2D(largura, altura)` leva o (0, 0) ao canto
  superior esquerdo da tela, o (-1, 1). Com o Y para baixo, um ângulo positivo em torno do Z gira no
  sentido horário na tela.
- Transformações 2D: na `mat3`, com o ponto como `(x, y, 1)`, as do GLM nas duas pontas
  (`notas.md`, 4.15). O ponto e o `vec2` da mesma precisão convertem um no outro, explicitamente.
- Ponto e vetor (`notas.md`, 4.18): ponto − ponto dá um vetor, ponto ± vetor e ponto ± tamanho dão
  um ponto, e o retângulo se move por um vetor; ponto + ponto, ponto × k, ponto / k e −ponto não
  existem. `mat3 * ponto` é o ponto como `(x, y, 1)`, com a translação. O `Lerp` de dois pontos e o
  `PointAt` do segmento usam a fórmula do `glm::mix`, `a * (1 - t) + b * t`, no tipo do `t`; no
  `int32`, o `t` é `double` e o resultado trunca, como o `glm::mix` de um `ivec2`, e satura fora do
  intervalo, onde no GLM a conversão seria indefinida.
- O comprimento do vetor inteiro (`notas.md`, 4.17): `Length` e `Distance` em `double`, com cada
  lado ao quadrado em `double`, e `LengthSquared` e `DistanceSquared` exatos em 64 bits, que voltam
  ao contrário passando do `int64`; a diferença do `Distance` é calculada em 64 bits. O segmento
  inteiro mede pelo `Distance`; o `Direction` dele volta ao contrário como o ponto.
- Texto da matriz: as colunas na ordem da memória, cada uma no formato do vetor, como em
  `((1, 0, 0), (0, 1, 0), (0, 0, 1))`.
- Hex da cor: `0xRRGGBBAA`, a ordem em que o número é escrito, feita com deslocamento de bits. No
  C#, o `ToArgb()` em `0xAARRGGBB` existe só para o `System.Drawing`.
