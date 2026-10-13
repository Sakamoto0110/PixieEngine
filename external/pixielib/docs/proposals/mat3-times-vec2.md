# A Vector Through a mat3

**Document ID:** GLOBAL-PROPOSAL-MAT3-TIMES-VEC2

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** a mat3 times a vec2, as (x, y, 0), so that a direction turns and
scales without the translation

**Surface:** proposal

**Boundary:** math

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PXL-PROPOSAL-001

**State:** idea

**Idea:** Multiply a mat3 by a vec2 as (x, y, 0): the rotation, scale, and shear
apply and the translation does not, the counterpart of mat3 * point, which takes
the point as (x, y, 1).

**Rationale:** PixieEngine will need it to turn a velocity or a normal with its
object. Point and vector were separated on 2026-10-12 mainly so that a matrix
does the right thing with each; the point half exists, the vector half was
deferred until something needs it.

**Constraints:** In C++ both operands are GLM types, so an operator* for them
outside namespace glm is not found by argument-dependent lookup, and it
disappears inside any namespace that declares another operator*. The form (an
operator in namespace glm, a named function, or glm::dmat2(m) * v) is to be
decided with the first use. Both sides must give the same bits.

**Promotion target:** owner decision required

**Source:** question 8.1, item 5, in section 4.18 of
[PXL-ARCHIVE-001](../archive/decision-notes-2026-10-12.md)