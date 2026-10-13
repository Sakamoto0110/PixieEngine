# Imported: PixieLib Handoff (pt-BR)

**Document ID:** ARCHIVE-PIXIELIB-HANDOFF-20261002

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the context handoff that started the C# side of PixieLib, written at the end of an InteractiveEditor rework session

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PXL-ARCHIVE-003

**Type:** import

**Origin:** owner decision: the documentation migration of 2026-10-12 (PXL-TODO-001)

**Author:** claude

**Reference date:** 2026-10-02

**Commits:** with this record (parent `e0186c65321fd112d389bdabd11a6e2b3d748a6d`)

**Reference commit:** `7a43f42a6b1d784ce3051b13ce8e5e3bd91857a7`, the last commit that changed the document

**Original path:** `docs/passagem-pixielib.md`, in this repository

**Current state:** What PixieLib is, its decisions, and how to commit are in [PXL-ARCHIVE-001](decision-notes-2026-10-12.md) and in the current owners listed there

The body below is the document as it stood at the reference commit, unchanged and in Portuguese
(pt-BR). It predates the [documentation policy](../governance/documentation-policy.md), which was
adopted on 2026-10-12; current documents are in English. PixieInspector keeps another copy of this
handoff as its PXI-ARCHIVE-003; this is the copy of the repository it was written for. Its questions
(section 6) were answered in [PXL-ARCHIVE-001](decision-notes-2026-10-12.md), and its account of the
repository describes the 2023 code. Current facts were extracted, in English, to their current
owners; where this record and a current owner disagree, the current owner wins.

---

# Passagem de contexto: a PixieLib em C#

Para começar a parte em C# da PixieLib num contexto novo. Escrita em 2 de outubro de 2026, no fim de
uma sessão do InteractiveEditor Rework, a pedido do void ("dois handoffs, um para o projeto da
PixieLib"), a partir das notas do rework e de uma leitura do repositório da PixieLib. Nada da parte
em C# existe ainda: o protótipo de 26 e 27/09 foi feito fora de qualquer repositório e se perdeu com
aquela sessão; o que sobrou dele são as conclusões, na seção 4. Ler isto inteiro antes de mexer em
qualquer coisa.

Esta passagem não é o começo. Antes dela, o void põe no repositório o contexto da PixieLib: a
história, por que ela foi feita daquele jeito, outros repositórios com informação relevante e o
jeito dele de programar. Depois, os dois definem o que a PixieLib é e, principalmente, o que ela não
é; é nessa definição que esta passagem entra. Onde o contexto do void e esta passagem divergirem,
vale o contexto: esta foi escrita olhando só o C++ e as notas do rework.

Esta passagem fica no repositório do InteractiveEditor Rework
(`Sakamoto0110/InteractiveEditor_Rework`, branch `rework-claude`, `docs/passagem-pixielib.md`),
porque a sessão que a escreveu não tinha permissão de escrita na PixieLib (seção 1). O lugar dela é
o repositório da PixieLib, quando houver acesso ou o void a puser lá.

---

## 1. Antes de qualquer commit

Regras do void, as mesmas do rework: **só subir para o GitHub se o author for ele**.

- Author e committer: `Rafael Sakamoto <rafael.sakamoto1@hotmail.com>`. Num clone novo, antes do
  primeiro commit:

  ```
  git config user.name "Rafael Sakamoto"
  git config user.email "rafael.sakamoto1@hotmail.com"
  git config commit.gpgsign false
  ```

- No fim da mensagem, os trailers de coautoria e de sessão que a própria sessão indicar.
- Depois do push, conferir no GitHub que os commits aparecem com o login `Sakamoto0110`, duas linhas
  por commit (author e committer), com `N` sendo quantos commits olhar e `BRANCH` a branch usada:

  ```
  repo=Sakamoto0110/PixieLib
  curl -s "https://api.github.com/repos/$repo/commits?sha=BRANCH&per_page=N" | grep '"login"'
  ```

- A branch: **perguntar ao void antes do primeiro push**. O repositório tem a `master` e a
  `indev`; no rework, a regra é a `rework-claude`, mesmo que a sessão sugira outra, e o natural
  seria repetir a regra aqui, mas isso é ele quem decide.
- O acesso: em 02/10, a leitura funcionou, mas o push foi recusado, porque o app do Claude no GitHub
  não tinha permissão de escrita na PixieLib. O void resolve instalando o app no repositório ou
  reconectando o GitHub em https://claude.ai/connect-github.
- Mensagens de commit em inglês, com prefixo: `(refactor)`, `(docs)`, `(fix)`, `(feat)`. Mudança de
  código e atualização das notas em commits separados, o de `(docs)` citando o hash do outro.

## 2. Como o void trabalha

- Nunca chamar de Rafael: esse nome só aparece na identidade do git (seção 1) e nunca foi usado na
  conversa; quando o próprio nome aparece, é o sobrenome, Sakamoto. Chamar de void no código de mais
  baixo nível, mesmo em C#, e de neko quando o assunto é construir ferramentas e frameworks; na
  dúvida, void.
- Conversa em português, sem emojis. Notas em português, com linhas de até 100 colunas e referências
  a commits; código e comentários do código em inglês.
- O foco é simplificar ao máximo, mas sem juntar arquivos só para diminuir a contagem: um conceito
  por arquivo.
- Redesenhar com calma, uma coisa por vez, e tratar a causa em vez de remendar sintomas.
- Decisão dele vai logo para as notas, e o que é só proposta fica marcado como proposta. Mudança de
  desenho só depois de ele confirmar; quando ele pedir, aplicar, verificar e subir.
- Ele responde as perguntas pelo número ("2.3: sim"), às vezes com texto livre. Quando pede opinião,
  dar uma recomendação, não uma lista de opções; quando pede "opinião sincera", dizer também o custo
  do que se recomenda.
- Os nomes que ele cita são exemplos, não para levar ao pé da letra.
- O ideal é uma DLL só. Se aparecer outra, dizer para que ela serve. (A PixieLib vai ser justamente
  a segunda DLL de quem usa o InteractiveEditor; ver a pergunta 6.6.)
- Antes de dar uma mudança por pronta, conferir que os testes pegam o erro: desfazer a mudança (ou
  quebrar de propósito uma cópia) e ver os testes novos falharem.

## 3. O repositório hoje

- `Sakamoto0110/PixieLib`, parado desde agosto de 2023: 32 commits, de 24/07 a 20/08/2023. A
  `master` está em `328b62d` (20/08/2023, o merge do PR 3, "indev-less-pointers"); a `indev` está em
  `b961405` (17/08/2023) e tem commits que a `master` não tem (mexem em `pxEventsEx.hpp`,
  `pxTests.hpp` e tiram o `main.cpp`).
- Só C++: `PixieLib.sln` (Visual Studio 17) e `cpp/Demo/Pixielib/`, com o `Pixielib.vcxproj`, um
  `main.cpp` e, em `include/`: `pxCorelib.h` (os tipos base), `VectorMath.h` (as contas de vetor
  2D), `pxEvents.hpp` (811 linhas: callbacks e um `pxEventHandler`, com ou sem STL, por
  `USE_STL`/`NOT_USE_STL`), `pxEventsEx.hpp`, `pxContainers.hpp` (um `pxArray` com política de
  crescimento), `pxStrings.hpp`, `pxTests.hpp` e `_pxDebug.hpp`. O `README.md` tem só o título.
- Os nomes do C++, em `pxCorelib.h`: prefixo `px` (as macros `PIXIE_SHORT_TYPENAME px` e
  `PIXIE_LONG_TYPENAME Pixie`). `pxPoint_t<T>` e `pxSize_t<T>` são modelos, com os apelidos
  `pxPoint` (**int**), `pxPointf` (float) e `pxPointd` (double), e o mesmo para `pxSize`. `pxColor`
  tem `red`, `green`, `blue` e `alpha` em bytes, com conversão de e para um `unsigned int` na ordem
  RGBA. `pxVec2` e `pxVec3` são só em double, sem sufixo, e o namespace `Vec2` tem `Add`, `Sub`,
  `Mult`, `Div`, `Mag`, `Dot`, `AngleBetween` e `Normalize`.
- **O conflito de nomes**: no C++, o nome sem sufixo é int nos pontos e tamanhos (`pxPoint`) e
  double nos vetores (`pxVec2`). No C#, a precisão padrão decidida é double (P8.6), e o
  InteractiveEditor já usa `PxPoint` sem sufixo em double. É a pergunta 6.1.

## 4. O que já foi decidido

As decisões estão nas notas do rework (`docs/notas-modernizacao.md` do `InteractiveEditor_Rework`,
branch `rework-claude`): a seção 3.9 inteira, a seção 0 ("Primitivos e PixieLib") e as respostas
P8.1 a P8.9. Em resumo:

- **A ideia**: os primitivos do InteractiveEditor saem dele e viram a base da PixieLib em C#, que
  depois ganha vetores, matrizes e transformações 2D e 3D. Enquanto a PixieLib não existe, os
  primitivos ficam no InteractiveEditor, sem `PixieLib.dll` (P8.7, P8.8); a mudança para cá é esta
  sessão própria.
- **Nomes** (decidido): o prefixo `Px`, espelhando o `px` do C++ (`PxPoint`, `PxSize`, `PxVec2`,
  `PxMat3`...); as cores são `PxColorArgb` e `PxColorHsl`, com o espaço de cor no fim (P8.3).
  Testado: não colide com `System.Drawing`, `System.Numerics` nem SkiaSharp (CS0104), todos
  importados no mesmo arquivo.
- **Alvos** (decidido): a NekoLib vai usar a PixieLib também no .NET Framework, então ela compila
  para `net481` além do .NET moderno.
- **Precisões** (decidido): float, double e int saem de um modelo só, por um source generator
  (`PxVec2f`, `PxVec2d`, `PxVec2i`), porque a matemática genérica (`INumber<T>`) não existe no
  `net481` (testado). O modelo usa marcadores (`__S__` para o sufixo, `__T__` para o tipo) e blocos
  `#if` por tipo (`PX_FLOAT`, `PX_FLOATING`, `PX_INT`); no protótipo, a biblioteca compilou nos dois
  alvos, e o `PxVec2i` não tem `Length()`.
- **Repasse ao `System.Numerics`**: a versão float repassa as operações ao `Vector2`, `Vector3`,
  `Matrix3x2` e `Matrix4x4`, e isso sai de graça: uma função própria que chama `Vector2.Add` gera o
  mesmo `vaddps` que chamar direto (testado no assembly do JIT). O `System.Numerics` só tem float,
  então double e int são implementação própria.
- **A precisão padrão** é double (P8.6, 27/09), como o `Vec2` do C++; o InteractiveEditor passou
  tudo para double (P8.1, commit `2951ce3`).
- **As regras de conversão** (P8.5): implícita quando não perde nada, explícita quando perde ou pode
  lançar.
- **Proposta, não decidido**: o código em C# numa pasta `dotnet/` ao lado da `cpp/`, no mesmo
  repositório (uma biblioteca em duas linguagens, e não "duas PixieLib"); no NuGet, o nome
  `PixieLib` estava livre em 27/09, e `Pixie` não.
- **Proposta, não decidido**: `PxPoint` e `PxSize` sobre o `PxVec2`, o mesmo dado (8 bytes em
  float), com nomes e só as operações que fazem sentido: ponto + tamanho → ponto, ponto + vetor →
  ponto, ponto − ponto → vetor, tamanho + tamanho → tamanho, tamanho × k → tamanho; conversão
  implícita para o `PxVec2` (a matemática vem dele) e explícita de volta, testadas no protótipo.

## 5. O que o InteractiveEditor tem hoje, e que vai mudar de lugar

Em `InteractiveEditor/Primitives`, no namespace `InteractiveEditor.Primitives` (commits `21cbeda`,
`9e15f6e`, `2951ce3`, `3544a8e`, `f1de920` e `ccc3a5d`):

- `PxPoint` (`X`, `Y`), `PxSize` (`Width`, `Height`), `PxRect` (`X`, `Y`, `Width`, `Height`,
  `Right`, `Bottom`, `Contains`) e `PxPadding` (`Left`, `Top`, `Right`, `Bottom`, `Horizontal`,
  `Vertical`), todos em double, structs com `Empty`, `IsEmpty`, igualdade e um `ToString` que não
  depende da cultura; `PxPoint` e `PxSize` com a aritmética deles (`+`, `-`, `*` e `/` por escalar,
  ponto ± tamanho).
- `PxDock`, um enum (`None`, `Top`, `Bottom`, `Left`, `Right` e `Fill`, os valores do `DockStyle`).
- `PxColorArgb` (bytes `A`, `R`, `G`, `B`; construtores RGB, ARGB e por `int`; `ToArgb()`) e
  `PxColorHsl` (`A` em byte, `H`, `S` e `L` em double), sem conversão implícita entre as duas: as
  funções estáticas `PxColorHsl.FromArgb`/`ToArgb` e `PxColorArgb.FromHsl`/`ToHsl` (P8.4).
- Conversões com o `System.Drawing` (`Point`, `PointF`, `Size`, `SizeF`, `Rectangle`, `RectangleF` e
  `Color`), nos dois alvos, porque o `System.Drawing.Primitives` existe fora do Windows: implícitas
  dele para o `Px`, explícitas de volta (o int arredonda, como o `Point.Round`; o float estreita);
  com o `Color`, implícitas nos dois sentidos.
- Conversões com o WinForms e o WPF (commit `ccc3a5d`), em arquivos parciais `*.Windows.cs`, que só
  o alvo `net10.0-windows` compila: o `Point` do WPF, implícito nos dois sentidos; o `Size` e o
  `Rect` do WPF, implícitos deles e explícitos de volta (eles lançam com largura ou altura
  negativa); o `Padding` do WinForms, implícito dele e explícito de volta, arredondando; a
  `Thickness` e a `Color` do WPF, implícitas nos dois sentidos; e o `DockStyle` por métodos de
  extensão (`ToDockStyle()` e `ToPxDock()`), porque um enum não declara conversões, nem num bloco
  `extension` (CS9282).
- Quem usa: o passo de layout (`PxRect`, `PxSize`, e o `PxPadding` no `InspectorOptions.Padding`), a
  view WinForms (que converte o `PxRect` do layout em `Rectangle`) e o editor de cor (`PxColorArgb`,
  `PxColorHsl` e `System.Drawing.Color`).
- Os testes: o `Primitives.cs` do console de testes do rework (`probe`) e o `probe-windows` (as 21
  checagens das conversões, rodando no Wine), no `console-de-testes.zip` que o void tem.

## 6. Perguntas para levar ao void

Nenhuma tem resposta ainda. A sugestão vem no fim de cada uma, como proposta.

1. **O sufixo** (P8.9). No C++, o nome sem sufixo dos pontos e tamanhos é int; no C#, o padrão
   decidido é double, e o InteractiveEditor já usa `PxPoint` em double. Sugestão: no C#, o nome sem
   sufixo é o double, e os outros levam `f` e `i` (`PxPoint`, `PxPointf`, `PxPointi`), já que o
   gerador produz os três de um modelo; o lado C++ se alinha quando for mexido. Uma biblioteca em
   duas linguagens deveria querer dizer a mesma coisa com o mesmo nome, e o double é o padrão
   decidido nas duas pontas (o `pxVec2` do C++ já é double).
2. **Onde** (seção 4): a pasta `dotnet/` no repositório da PixieLib. Sugestão: sim, com uma solução
   própria lá dentro, sem mexer na `PixieLib.sln` do C++.
3. **A branch e o acesso** (seção 1).
4. **As conversões com o WinForms e o WPF.** Um operador de conversão tem que estar no próprio tipo,
   então, se os tipos forem para a PixieLib, essas conversões vão junto, e a PixieLib precisa de um
   alvo `-windows` como o do InteractiveEditor; ou elas viram métodos de extensão no
   InteractiveEditor, e se perde o `(Rectangle)rect`. Sugestão: a PixieLib com os alvos `net481`,
   `net10.0` e `net10.0-windows`, as conversões de Windows em `*.Windows.cs`, como no
   InteractiveEditor; no `net481` o WinForms e o WPF fazem parte do framework, então ele recebe as
   conversões também.
5. **Quais primitivos vão.** O C++ tem pontos, tamanhos, vetores e uma cor RGBA; o InteractiveEditor
   tem também retângulo, margens, a doca e as cores ARGB e HSL. Sugestão: todos vão, e o `PxDock`,
   que é coisa de interface, também, porque o `PxPadding` e o `PxRect` vão; a cor do C++ fica para
   quando o C++ for mexido.
6. **A segunda DLL.** Com a PixieLib, quem usa o InteractiveEditor recebe duas DLLs. Sugestão: dizer
   isso ao void (é a regra "se aparecer outra, dizer para que ela serve") e manter a PixieLib como
   pacote próprio, porque a NekoLib também vai usá-la.
7. **O gerador.** O source generator é um projeto de analisador, que roda na compilação e não vai
   junto como DLL de quem usa. Sugestão: um projeto `PixieLib.Generators` dentro de `dotnet/`,
   referenciado como analisador pelo projeto da biblioteca.
8. **A troca no InteractiveEditor**: quando a PixieLib existir, o InteractiveEditor passa a
   referenciá-la e apaga os seus primitivos; o namespace muda de `InteractiveEditor.Primitives` para
   o da PixieLib. Sugestão: fazer isso numa sessão do rework, depois desta, para não disputar o
   mesmo código com os cortes das views que estão em andamento lá (a view WPF ainda vai usar os
   primitivos).

## 7. Como verificar

- O `dotnet` 10 pode não estar instalado; o script oficial instala em `~/.dotnet`:

  ```
  curl -sSL -o dotnet-install.sh https://dot.net/v1/dotnet-install.sh
  bash dotnet-install.sh --channel 10.0 --install-dir "$HOME/.dotnet"
  export PATH="$HOME/.dotnet:$PATH" DOTNET_ROOT="$HOME/.dotnet"
  ```

- O `net481` compila no Linux com o pacote `Microsoft.NETFramework.ReferenceAssemblies`, mas não
  roda; o protótipo foi testado só compilando nos dois alvos. Para rodar código de Windows
  (WinForms, WPF, ou um `net481`), o rework usa o Wine; a receita, com as pegadinhas, está na seção
  4 da passagem do rework (`docs/passagem-de-contexto.md`).
- Testes num console pequeno fora do repositório, como no rework, e a conferência de que eles pegam
  o erro.
- Quando o InteractiveEditor passar a usar a PixieLib (pergunta 6.8), nada pode mudar nele: o build
  da solução com 0 warnings e 0 erros nos 6 projetos, o TuxHost com as mesmas 67 linhas, e os
  consoles de teste do rework (`probe`, `probe-windows` e `probe-view`) passando inteiros.

## 8. Próximo passo

1. Ler o contexto que o void pôs no repositório; depois isto, a seção 3.9 e a seção 0 das notas do
   rework, e os cabeçalhos do C++ (`pxCorelib.h` e `VectorMath.h`).
2. Definir com o void o que a PixieLib é e o que ela não é, com as seções 3 a 5 como ponto de
   partida, e registrar a definição no repositório antes de escrever código.
3. Levar ao void as perguntas da seção 6 que a definição não respondeu, pelo número, com as
   sugestões, e registrar as respostas.
4. Depois das respostas: o gerador e um primeiro tipo (`PxVec2` nas três precisões), compilando no
   `net481` e no `net10.0`, com testes; em seguida os primitivos do InteractiveEditor, com as
   conversões e os mesmos testes que eles têm lá.
