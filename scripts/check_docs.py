"""Checks PixieEngine's versioned documentation against the documentation policy and schema.

Run from anywhere: python scripts/check_docs.py
Exits 0 when every check passes, 1 otherwise, printing one line per problem as path:line: message.

Checks:
- relative links resolve, and their anchors exist in the target Markdown file;
- documents with metadata carry the fields the schema requires, with values from its vocabulary;
- TODO.md entries and TASKLIST.md sections point at each other, task IDs are unique, and dropped
  tasks state a reason;
- the archive router lists every record once, under the record's own Archive ID, in order.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# Generated, third-party, or unversioned trees; their Markdown is not ours to check.
SKIPPED_DIRS = {".git", ".vs", ".vscode", ".local", "build", "build-ci", "bin", "out", "x64", "external", "obj"}

problems: list[str] = []


def report(path: Path, line: int, message: str) -> None:
    problems.append(f"{path.relative_to(ROOT).as_posix()}:{line}: {message}")


def markdown_files() -> list[Path]:
    files = []
    for path in sorted(ROOT.rglob("*.md")):
        parts = path.relative_to(ROOT).parts
        if not any(part in SKIPPED_DIRS for part in parts[:-1]):
            files.append(path)
    return files


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def without_fences(text: str) -> str:
    """Blanks fenced code blocks, keeping line numbers."""
    return re.sub(r"```.*?```", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)


def without_code(text: str) -> str:
    """Blanks fenced code blocks and inline code, keeping line numbers."""
    return re.sub(r"`[^`\n]*`", lambda m: " " * len(m.group(0)), without_fences(text))


# --- Links and anchors ---------------------------------------------------------------------------

def slug(heading: str) -> str:
    """GitHub's anchor for a heading: lower case, punctuation dropped, spaces to hyphens."""
    heading = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", heading.strip())
    heading = heading.lower()
    heading = re.sub(r"[^\w\- ]", "", heading)
    return heading.replace(" ", "-")


_anchor_cache: dict[Path, set[str]] = {}


def anchors(path: Path) -> set[str]:
    if path not in _anchor_cache:
        found: set[str] = set()
        counts: dict[str, int] = {}
        for line in without_fences(read(path)).splitlines():
            match = re.match(r"^#{1,6}\s+(.*?)\s*#*\s*$", line)
            if match:
                base = slug(match.group(1))
                n = counts.get(base, 0)
                found.add(base if n == 0 else f"{base}-{n}")
                counts[base] = n + 1
        _anchor_cache[path] = found
    return _anchor_cache[path]


def check_links(path: Path) -> None:
    text = without_code(read(path))
    for match in re.finditer(r"\]\(<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\)", text):
        target = match.group(1)
        line = text.count("\n", 0, match.start()) + 1
        if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):
            continue  # an external URL or mailto:
        file_part, _, fragment = target.partition("#")
        resolved = (path.parent / file_part).resolve() if file_part else path
        if not resolved.exists():
            report(path, line, f"link target does not exist: {target}")
            continue
        try:
            resolved.relative_to(ROOT)
        except ValueError:
            report(path, line, f"link leaves the repository: {target}")
            continue
        if file_part and any(part == ".local" for part in resolved.relative_to(ROOT).parts):
            report(path, line, f"versioned documents never link into .local/: {target}")
        if fragment and resolved.suffix == ".md" and fragment not in anchors(resolved):
            report(path, line, f"anchor not found: {target}")


# --- Metadata ------------------------------------------------------------------------------------

def metadata(path: Path) -> dict[str, tuple[str, int]]:
    """Bold-label fields before the first section heading: label -> (value, line)."""
    fields: dict[str, tuple[str, int]] = {}
    for number, line in enumerate(without_fences(read(path)).splitlines(), start=1):
        if line.startswith("## "):
            break
        match = re.match(r"^\*\*([^*]+):\*\*\s*(.*)$", line)
        if match:
            fields[match.group(1)] = (match.group(2).strip(), number)
    return fields


def check_metadata(path: Path, schema: dict, document_ids: dict[str, Path]) -> None:
    fields = metadata(path)
    if "Document ID" not in fields:
        return  # not an indexable document
    rules = schema["metadata"]
    for name in rules["requiredForExtendedSchema"]:
        if name not in fields:
            report(path, 1, f"missing metadata field: {name}")
    for name, allowed in rules["fields"].items():
        if name in fields and fields[name][0] not in allowed:
            value, line = fields[name]
            report(path, line, f"{name} '{value}' is not one of: {', '.join(allowed)}")

    doc_id, line = fields["Document ID"]
    if not re.match(rules["documentIdPattern"], doc_id):
        report(path, line, f"Document ID '{doc_id}' does not match {rules['documentIdPattern']}")
    if doc_id in document_ids:
        report(path, line, f"Document ID '{doc_id}' is also used by "
                           f"{document_ids[doc_id].relative_to(ROOT).as_posix()}")
    document_ids[doc_id] = path
    if "Boundary" in fields and not re.match(rules["boundaryPattern"], fields["Boundary"][0]):
        report(path, fields["Boundary"][1], f"Boundary '{fields['Boundary'][0]}' does not match "
                                            f"{rules['boundaryPattern']}")
    if "Schema version" in fields and fields["Schema version"][0] != str(schema["schemaVersion"]):
        report(path, fields["Schema version"][1], f"Schema version must be {schema['schemaVersion']}")

    surface = fields.get("Surface", ("", 0))[0]
    surface_rules = schema["surfaceRules"].get(surface, {})
    role = fields.get("Authority role", ("", 0))[0]
    if "allowedAuthorityRoles" in surface_rules and role not in surface_rules["allowedAuthorityRoles"]:
        report(path, fields["Authority role"][1], f"Authority role '{role}' is not allowed for "
                                                  f"Surface {surface}")
    for name in surface_rules.get("requiredMetadata", []):
        if name not in fields:
            report(path, 1, f"Surface {surface} requires metadata field: {name}")
    lifecycle = fields.get("Lifecycle", ("", 0))[0]
    if "requiredLifecycle" in surface_rules and lifecycle != surface_rules["requiredLifecycle"]:
        report(path, fields["Lifecycle"][1], f"Surface {surface} requires Lifecycle "
                                             f"{surface_rules['requiredLifecycle']}")
    mutation = fields.get("Mutation", ("", 0))[0]
    if "requiredMutation" in surface_rules and mutation != surface_rules["requiredMutation"]:
        report(path, fields["Mutation"][1], f"Surface {surface} requires Mutation "
                                            f"{surface_rules['requiredMutation']}")
    if lifecycle == "current":
        for name in surface_rules.get("forbiddenMetadataWhenCurrent", []):
            if name in fields:
                report(path, fields[name][1], f"a current {surface} must not carry {name}")

    if surface == "archive-record":
        archive = schema["archive"]
        if "Type" in fields and fields["Type"][0] not in archive["types"]:
            report(path, fields["Type"][1], f"Type '{fields['Type'][0]}' is not one of: "
                                            f"{', '.join(archive['types'])}")
        if "Author" in fields and fields["Author"][0] not in archive["authors"]:
            report(path, fields["Author"][1], f"Author '{fields['Author'][0]}' is not one of: "
                                              f"{', '.join(archive['authors'])}")
        if "Archive ID" in fields and not re.match(archive["idPattern"], fields["Archive ID"][0]):
            report(path, fields["Archive ID"][1], f"Archive ID '{fields['Archive ID'][0]}' does not "
                                                  f"match {archive['idPattern']}")
        if "Reference date" in fields and not re.match(r"^\d{4}-\d{2}-\d{2}$",
                                                       fields["Reference date"][0]):
            report(path, fields["Reference date"][1], "Reference date must be YYYY-MM-DD")


# --- Schedulers ----------------------------------------------------------------------------------

TASK_LINE = re.compile(r"^- `(?P<id>[A-Z][A-Z0-9.]*-TASK-\d{3})` — \*\*Status:\*\* (?P<status>[a-z-]+) — "
                       r"(?P<rest>.+)$")


def sections(text: str, level: str) -> list[tuple[str, str, int]]:
    """(heading, body, line of the heading) for each heading of the given level."""
    pattern = re.compile(rf"^{level} (.+)$", re.M)
    found = list(pattern.finditer(text))
    out = []
    for i, match in enumerate(found):
        end = len(text)
        if i + 1 < len(found):
            end = found[i + 1].start()
        shallower = re.compile(rf"^#{{1,{len(level) - 1}}} ", re.M).search(text, match.end(), end)
        if shallower:
            end = shallower.start()
        out.append((match.group(1), text[match.end():end], text.count("\n", 0, match.start()) + 1))
    return out


def field(body: str, name: str) -> str | None:
    match = re.search(rf"^\*\*{re.escape(name)}:\*\*\s*(.*)$", body, re.M)
    return match.group(1).strip() if match else None


def check_task_lines(path: Path, body: str, first_line: int, schema: dict,
                     task_ids: dict[str, str]) -> None:
    rules = schema["schedulers"]["tasklist"]
    for offset, line in enumerate(body.splitlines()):
        if not line.startswith("- `"):
            continue
        number = first_line + offset
        match = TASK_LINE.match(line)
        if not match:
            report(path, number, "task line does not follow '- `ID` — **Status:** status — description'")
            continue
        task_id, status = match.group("id"), match.group("status")
        where = f"{path.relative_to(ROOT).as_posix()}:{number}"
        if task_id in task_ids:
            report(path, number, f"task ID {task_id} is also defined at {task_ids[task_id]}")
        task_ids[task_id] = where
        if status not in rules["statuses"]:
            report(path, number, f"task status '{status}' is not one of: {', '.join(rules['statuses'])}")
        if status == "dropped" and "**Reason:**" not in match.group("rest"):
            report(path, number, f"dropped task {task_id} must state a **Reason:**")


def check_schedulers(schema: dict, task_ids: dict[str, str]) -> None:
    todo_path, tasklist_path = ROOT / "TODO.md", ROOT / "TASKLIST.md"
    todo_rules = schema["schedulers"]["todo"]
    todo_text, tasklist_text = read(todo_path), read(tasklist_path)

    entries: dict[str, tuple[str, int]] = {}  # TODO ID -> (heading, line)
    breakdowns: dict[str, str] = {}
    for heading, body, line in sections(todo_text, "###"):
        match = re.match(r"^`([A-Z][A-Z0-9.]*-TODO-\d{3})` — ", heading)
        if not match:
            continue
        todo_id = match.group(1)
        if not re.match(todo_rules["idPattern"], todo_id):
            report(todo_path, line, f"TODO ID {todo_id} does not match {todo_rules['idPattern']}")
        if todo_id in entries:
            report(todo_path, line, f"{todo_id} appears twice")
        entries[todo_id] = (heading, line)
        for name in todo_rules["requiredFields"]:
            if field(body, name) is None:
                report(todo_path, line, f"{todo_id} lacks the field {name}")
        status = (field(body, "Status") or "").split()[0] if field(body, "Status") else ""
        if status and status not in todo_rules["statuses"]:
            report(todo_path, line, f"{todo_id} status '{status}' is not one of: "
                                    f"{', '.join(todo_rules['statuses'])}")
        breakdown = field(body, "Breakdown")
        if breakdown and breakdown != todo_rules["breakdownEmptyValue"]:
            target = re.search(r"\(TASKLIST\.md#([^)]+)\)", breakdown)
            if not target:
                report(todo_path, line, f"{todo_id} Breakdown must link to a TASKLIST.md section or be none")
            else:
                breakdowns[todo_id] = target.group(1)

    parents: dict[str, str] = {}  # TODO ID -> section anchor
    for heading, body, line in sections(tasklist_text, "##"):
        match = re.match(r"^`([A-Z][A-Z0-9.]*-TODO-\d{3})` — ", heading)
        if not match:
            continue
        todo_id = match.group(1)
        parent = field(body, "Parent")
        if parent is None or todo_id not in parent:
            report(tasklist_path, line, f"section {todo_id} must name {todo_id} in its Parent field")
        if todo_id not in entries:
            report(tasklist_path, line, f"section {todo_id} has no entry in TODO.md")
        parents[todo_id] = slug(heading)
        check_task_lines(tasklist_path, body, line, schema, task_ids)

    for todo_id, anchor in breakdowns.items():
        if parents.get(todo_id) != anchor:
            report(todo_path, entries[todo_id][1], f"{todo_id} Breakdown does not point at its "
                                                   f"TASKLIST.md section")
    for todo_id in parents:
        if todo_id in entries and todo_id not in breakdowns:
            report(todo_path, entries[todo_id][1], f"{todo_id} has a TASKLIST.md section but its "
                                                   f"Breakdown does not link to it")


# --- Archive router ------------------------------------------------------------------------------

def check_archive(schema: dict, task_ids: dict[str, str]) -> None:
    directory = ROOT / "docs" / "archive"
    router = directory / "README.md"
    records: dict[str, Path] = {}
    for path in sorted(directory.glob("*.md")):
        if path == router:
            continue
        fields = metadata(path)
        if "Archive ID" not in fields:
            report(path, 1, "archive record without an Archive ID")
            continue
        archive_id = fields["Archive ID"][0]
        if archive_id in records:
            report(path, fields["Archive ID"][1], f"{archive_id} is also used by "
                                                  f"{records[archive_id].name}")
        records[archive_id] = path
        # Completed breakdowns keep their task lines; their IDs stay taken.
        text = read(path)
        for heading, body, line in sections(text, "##"):
            if heading.startswith("Breakdown"):
                check_task_lines(path, body, line, schema, task_ids)

    rows: list[tuple[str, str, int]] = []
    for number, line in enumerate(read(router).splitlines(), start=1):
        match = re.match(r"^\| ([A-Z][A-Z0-9.]*-ARCHIVE-\d{3}) \|.*\]\(([^)]+)\) \|\s*$", line)
        if match:
            rows.append((match.group(1), match.group(2), number))
        elif re.match(r"^\| [A-Z][A-Z0-9.]*-ARCHIVE-", line):
            report(router, number, "router row must end with a link to its record")
    listed: set[str] = set()
    previous = 0
    for archive_id, link, number in rows:
        record = (directory / link).resolve()
        if archive_id in listed:
            report(router, number, f"{archive_id} has more than one row")
        listed.add(archive_id)
        sequence = int(archive_id.rsplit("-", 1)[1])
        if sequence <= previous:
            report(router, number, f"{archive_id} is out of Archive ID order")
        previous = sequence
        if records.get(archive_id) != record:
            owner = records.get(archive_id)
            actual = owner.name if owner else "no record"
            report(router, number, f"{archive_id} links to {link}, but that ID belongs to {actual}")
    for archive_id, path in records.items():
        if archive_id not in listed:
            report(path, 1, f"{archive_id} has no row in docs/archive/README.md")


def main() -> int:
    schema = json.loads(read(ROOT / "docs" / "schemas" / "documentation-schema.json"))
    files = markdown_files()
    document_ids: dict[str, Path] = {}
    for path in files:
        check_links(path)
        check_metadata(path, schema, document_ids)
    task_ids: dict[str, str] = {}
    check_schedulers(schema, task_ids)
    check_archive(schema, task_ids)

    for problem in problems:
        print(problem)
    print(f"{len(files)} documents checked, {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
