# Imported: Open Questions (pt-BR)

**Document ID:** ARCHIVE-OPEN-QUESTIONS-20261012

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the pre-policy open-questions file, with every question answered: a router from each section of questions to the section of the notes that holds the answers

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PXL-ARCHIVE-002

**Type:** import

**Origin:** owner decision: the documentation migration of 2026-10-12 (PXL-TODO-001)

**Author:** claude

**Reference date:** 2026-10-12

**Commits:** with this record (parent `e0186c65321fd112d389bdabd11a6e2b3d748a6d`)

**Reference commit:** `e0186c65321fd112d389bdabd11a6e2b3d748a6d`, the last commit that changed the document

**Original path:** `docs/perguntas-em-aberto.md`, in this repository

**Current state:** Open questions now live in the unversioned `.local/decisions/open-questions.md`, or in the conversation where `.local/` does not survive; answers are promoted to a versioned owner

The body below is the document as it stood at the reference commit, unchanged and in Portuguese
(pt-BR). It predates the [documentation policy](../governance/documentation-policy.md), which was
adopted on 2026-10-12; current documents are in English. It records the convention of answering by
number ("8.1: sim"), which the answers in [PXL-ARCHIVE-001](decision-notes-2026-10-12.md) follow,
and section 8, the question about the point and the vector, whose test results are in section 4.18
of that record. Current facts were extracted, in English, to their current owners; where this record
and a current owner disagree, the current owner wins.

---

# Perguntas em aberto

Estado de 12/10/2026. As respondidas saem daqui e vão para `notas.md`, citadas pelo número. Os
números não são reaproveitados. A sugestão, quando há, vem no fim de cada pergunta, e dá para
responder pelo número: "1.7: sim".

## 1. Definição e primeira rodada

Todas respondidas: da 1.1 à 1.6, a 1.9 e a 1.14 em 03/10, a 1.7, a 1.8, a 1.10 e a 1.15 em 08/10,
e da 1.11 à 1.13 em 09/10, com os vetores e as matrizes do GLM (`notas.md`, seções 1, 2 e 4).

## 2. Os primitivos em C++

Todas respondidas em 08/10, da 2.1 à 2.9 (`notas.md`, seções 4.9 e 4.10).

## 3. Os primitivos em C#

Todas respondidas em 09/10, da 3.1 à 3.5 (`notas.md`, seção 4.11).

## 4. Os eventos em C++

Todas respondidas: da 4.1 à 4.7 em 08/10, e a 4.8, as escolhas do `Forward`, em 09/10 (`notas.md`,
seção 4.6); a 4.6 trouxe o `Forward` de volta.

## 5. O 2D

Respondidas: da 5.1 à 5.7 em 08/10, e a 5.8 em 09/10 (`notas.md`, seção 4.12).

## 6. Vetores e matrizes

Todas respondidas em 09/10, da 6.1 à 6.7 (`notas.md`, seções 4.13 a 4.15).

## 7. O deploy

Todas respondidas em 09/10, da 7.1 à 7.4, feitas na conversa: a versão 0.1.0, a licença MIT, a
publicação por um workflow que o void dispara, e o CI com o Windows antes do deploy (`notas.md`,
seção 4.16).

## 8. O segmento e a separação entre ponto e vetor

Todas respondidas em 12/10, da 8.1 à 8.5 (`notas.md`, seções 4.17 e 4.18): o segmento primeiro, no
`pixie::math`, com as restrições do inteiro do GLM e a exceção do comprimento; sem o raio; e o
ponto separado do vetor, com a conversão explícita.

## 9. A implementação da seção 8

Respondida em 12/10: a 9.1, as escolhas do commit `a2c0eff`, todas aceitas (`notas.md`, seção
4.18).
