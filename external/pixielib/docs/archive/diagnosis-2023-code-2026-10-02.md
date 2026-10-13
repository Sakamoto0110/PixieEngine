# Imported: Diagnosis of the 2023 Code (pt-BR)

**Document ID:** ARCHIVE-DIAGNOSIS-2023-20261002

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** what the 2023 code actually did, checked with g++ 13.3 and the AddressSanitizer on copies outside the repository; nothing was fixed

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PXL-ARCHIVE-005

**Type:** import

**Origin:** owner decision: the documentation migration of 2026-10-12 (PXL-TODO-001)

**Author:** claude

**Reference date:** 2026-10-02

**Commits:** with this record (parent `e0186c65321fd112d389bdabd11a6e2b3d748a6d`)

**Reference commit:** `7a43f42a6b1d784ce3051b13ce8e5e3bd91857a7`, the last commit that changed the document

**Original path:** `docs/diagnostico-2023.md`, in this repository

**Current state:** The current code is described by the module references under [`docs/modules/`](../modules/)

The body below is the document as it stood at the reference commit, unchanged and in Portuguese
(pt-BR). It predates the [documentation policy](../governance/documentation-policy.md), which was
adopted on 2026-10-12; current documents are in English. Its paths are relative to
`cpp/Demo/Pixielib/include/` at the tag `legado-2023`. The code it describes was replaced when
PixieLib restarted (section 4.7 of [PXL-ARCHIVE-001](decision-notes-2026-10-12.md)). Current facts
were extracted, in English, to their current owners; where this record and a current owner disagree,
the current owner wins.

---

# Diagnóstico do código de 2023

O que o código antigo faz de fato, verificado em 02/10/2026: compilado com o g++ 13.3
(`-std=c++20`) e rodado com o AddressSanitizer, em cópias fora do repositório, com stubs no lugar
do `conio.h` e do `Windows.h`. Os testes eram descartáveis e não foram guardados. Os leaks já
conhecidos dos eventos não estão listados; o resto, sim. Nada foi corrigido.

Os caminhos da PixieLib são relativos a `cpp/Demo/Pixielib/include/`, na tag `legado-2023`
(`328b62d`).

---

## 1. Eventos (`pxEvents.hpp`)

A arquitetura é a do type erasure, a mesma que a `std::function` usa por dentro: a interface
`IEventCallback`, os callbacks concretos em modelos e o `pxCallback`, dono do ponteiro, com a cópia
pelo `copy()` virtual.

- **O `-=` não remove nada.** Depois de `h += f1; h += f2; h -= f2;`, o tamanho continua 2 e o `f2`
  continua sendo chamado. A deduplicação também não acontece: `h += f1` duas vezes chama o `f1`
  duas vezes. A causa: a identidade do callback é `(DWORD)&_fn` (linhas 334, 373 e 418), o endereço
  do membro que guarda o ponteiro, não o valor dele, e cada cópia mora num endereço diferente. Só o
  `chCallback` (linha 456) tem identidade certa. O porquê provável: ponteiro para membro não
  converte para inteiro; a `union u_ptm_cast` do `main.cpp` é dessa época.
- **Lambda com captura lê memória morta.** O `lbCallback_t` guarda `&f` (linha 399), e `f` é o
  parâmetro `_call` do construtor do `pxCallback` (linha 494), que morre quando o construtor
  retorna: stack-use-after-return no ASan. A lambda sem captura funciona por acaso, porque não lê o
  `this`.
- **`b += a` duas vezes destrói os callbacks do `a`.** Na segunda vez, a deduplicação acha o
  ponteiro que o primeiro join compartilhou e dá `delete` nele (linha 725), mas ele pertence ao
  `a`: o `a.Invoke()` vira heap-use-after-free.
- O modo `NOT_USE_STL` não compila (`IEventCallback**` sem argumentos de modelo, linha 276); só
  nunca foi ativado.
- `strlen` sem `<cstring>` (linha 98), `conio.h` (`pxEventsEx.hpp:48`), `_getch` e `system("cls")`
  (`pxTests.hpp`): o que impede de compilar fora do Windows.

O C# tem o mesmo problema da identidade com lambdas: lá, `-=` com uma lambda nova também não remove
nada, e é preciso guardar a instância.

## 2. Os outros arquivos da PixieLib

- **`pxContainers.hpp`**: o `_realloc` (linha 139) aloca um `tmp`, copia, libera, aloca de novo,
  copia de volta e nunca libera o `tmp` (com 100 `add`, 1020 bytes perdidos). O `_capacity` começa
  em 0, então o `initial_size` é ignorado. `calloc` e `memcpy` só valem para tipos triviais. Sem
  `operator=`, `a = b` dá double free. Como cresce, é um vetor, não um array.
- **`pxCorelib.h`**: o `PIXIE_TYPEDEF(Point, MAKE_PIXIE_LONG_TYPENAME, MAKE_PIXIE_SHORT_TYPENAME)`
  expande para `Point_t PixiePoint ,pxPoint`. A recursão com `__VA_OPT__` funciona (é a técnica do
  post de David Mazières sobre macros recursivas, com os mesmos nomes), mas falta a vírgula depois
  do primeiro nome, e o `typedef struct {...} PIXIE_TYPEDEF(...)` falha.
- **`VectorMath.h`**: a aritmética está certa. Os operadores estão no `namespace Vec2`, mas o
  `pxVec2` é global, e sem `using namespace` o `a + b` não compila. O `AngleBetween` usa `acos`, que
  não tem sinal. `Normalize({0, 0})` dá NaN.
- **`pxStrings.hpp`**: o truque do `ATOM<char>` funciona. O "Reason: ?????" da linha 54 são dois
  bugs que se cancelam: o `is_invocable` testa `char` mais `Ty...` (os parâmetros da função, 3
  argumentos) em vez de `Tz...` (os de runtime, 2), e o branch está invertido; o `goto` é
  redundante. O `Parse` só aceita um `int` e retorna `""`.
- **`pxTests.hpp`**: a mensagem da linha 68 diz "positive integer, including zero", mas o teste é
  `> 0`; "less then" é "less than". `Run<10000, 3>` roda 9999 vezes.
- **`_pxDebug.hpp`**: `std::map::insert(chave, valor)` (linha 44) não existe; quebra no primeiro
  uso.

## 3. vec2math (`Sakamoto0110/vec2math`, `aacdf7e`)

- **`atan2` com os argumentos trocados** em `Heading` e `heading` (linhas 97 e 124): é `(y, x)`.
  O heading de (1, 0) dá π/2, e o de (0, 1) dá 0. O candidato mais provável para o raylib.
- **Os concepts nunca ligaram**: `#ifdef __cplusplus 202002L` (linha 49) só olha o primeiro token
  e vira `#ifdef __cplusplus`, sempre verdadeiro, então o `NO_CONCEPTS` fica sempre definido.
- **`mag()`, `dot()`, `heading()`, `angleBetween()` e `normalize()` não podem ser chamados**: o
  `Decimal_t` só aparece no retorno e não é deduzido. `Vec2<int> * 0.5f` também não compila.
- **`#include <cmath>` dentro do `namespace vec2math`** (linha 28): com o header antes de qualquer
  `<cmath>`, 1641 erros.

## 4. pxCryptLib (`Sakamoto0110/pxCryptLib`, `c090255`)

- **`pxvt.h`**: a linha 180 (`T::_signature != vt::vtCurMOVEA`, sem o `::_signature`) quebra o g++.
  Um `main()` de rascunho com `<Windows.h>` no fim (linha 235); como o `StringBlock.h` inclui o
  `pxvt.h`, o demo tem dois `main`. O branch C++20 depende de `_MSVC_LANG`. As coordenadas não
  batem: `vtPosition(x, y)` põe a linha em `x`, `vtCursor<move>(x, y)` em `y`. O `operator<<`
  ignora o `out`. O `ESC[0m` do destrutor reseta tudo. `vtBuffMainA` e `vtBuffAltA` estão trocados
  (linhas 229 e 230: `?1049h` entra no buffer alternativo). Funções fora de modelo definidas no
  header, sem `inline`.
- **`pxCrypt_core.h`**: os tamanhos nos comentários não batem: `pxINT16` é `int` (4 bytes), e
  `long` tem 4 bytes no Windows e 8 no Linux de 64 bits.
- **`pxConsoleHelper`**: a `EnableVTMode()` nunca é chamada, e não é exportada da DLL (a
  declaração usa `pxCryptAPI_EXPORTS`, definido vazio, em vez de `pxCrypt_API`).
- **A cifra** (`StringBlock`): chave mais longa que a largura do bloco faz `cols[j]` sair do array
  (com "cgra" num bloco 2x2: heap-buffer-overflow). O `Dispose` usa `delete` em memória de `new[]`
  e de `calloc`.
- **A de César** (`pxEncryption.h`): funciona com chave de 0 a 25; com 26, `encode("abcxyz")` dá
  `` GHI^_` ``, porque o teste de intervalo usa `byte + key` sem reduzir.
- **`simpleArray.h`**: o construtor faz `_data[i] = new T; _data[i] = 0;`.

## 5. O padrão

Modelo que ninguém instancia não é verificado de verdade, em compilador nenhum: muito do que
"funcionava" compilava porque ninguém chamava (os métodos da vec2math, o `Encode` e o `Decode`, o
`Callable::Invoke`, o `_pxDebug`). A lição para a fase séria: testes que chamam tudo, e compilar em
mais de um compilador.
