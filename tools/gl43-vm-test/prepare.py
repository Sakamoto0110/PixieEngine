"""Package a local Windows x64 VM test from the two isolated Release install units."""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

sys.dont_write_bytecode = True
ENGINE = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ENGINE / "scripts/release"), str(ENGINE / "scripts/sdk")]

from assemble_sdk import compose, inventory, write_json
from prepare_release import pair_exporter
from rehearse_sdk import archive_tree, check_tree, digest, read_json, verify_and_relocate
from verify_sdk import verify


def replace_once(path: Path, replacements: dict[str, str]) -> None:
    source = path.read_text(encoding="utf-8")
    for old, new in replacements.items():
        if source.count(old) != 1:
            raise ValueError(f"Expected one occurrence of {old!r} in {path}")
        source = source.replace(old, new)
    path.write_text(source, encoding="utf-8", newline="\n")


def publish_editor(source: Path, copy: Path, output: Path, sdk: Path) -> None:
    copy.mkdir()
    for directory in ("src", "eng"):
        shutil.copytree(source / directory, copy / directory, ignore=shutil.ignore_patterns("bin", "obj", ".vs"))
    for name in ("global.json", "NuGet.Config", "version.json", "LICENSE"):
        shutil.copyfile(source / name, copy / name)
    replace_once(copy / "src/Ui/EditorWindow.cs", {
        "GLFW.GLFW_CONTEXT_VERSION_MINOR, 5": "GLFW.GLFW_CONTEXT_VERSION_MINOR, 3",
        "GLFW.GLFW_OPENGL_FORWARD_COMPAT, GLFW.GLFW_TRUE": "GLFW.GLFW_OPENGL_FORWARD_COMPAT, GLFW.GLFW_FALSE",
        '"PixieEditor", default(GLFWmonitorPtr)': '"PixieEditor [OpenGL 4.3 VM test]", default(GLFWmonitorPtr)',
        "an OpenGL 4.5 core context.": "an OpenGL 4.3 core context for the VM test.",
        'Init("#version 450")': 'Init("#version 430")',
    })
    replace_once(copy / "src/EditorSettings.cs", {
        '"PixieEditor", "settings.json"': '"PixieEditor-GL43-VM-Test", "settings.json"',
    })
    flags = [
        f"-p:PixieBuildSdkRoot={sdk}", f"-p:PixieBuildSdkCatalogSha256={digest(sdk / 'sdk.json')}",
        "-p:PixieUseLocalConfiguration=false", "-p:PixieDevelopmentTools=false",
        "-p:DebugType=None", "-p:DebugSymbols=false",
    ]
    for key, file in {
        "Binding": "bindings/dotnet/PixieCapi.cs", "Props": "bindings/dotnet/NativeApiVersion.props",
        "Metadata": "bindings/dotnet/NativeApiVersion.cs", "Header": "include/pixie_capi.h",
    }.items():
        flags.append(f"-p:PixieBuildSdk{key}Sha256={digest(sdk / file)}")
    subprocess.run(["dotnet", "publish", "src/PixieEditor.csproj", "-c", "Release",
                    "-r", "win-x64", "--self-contained", "true", "-o", output, *flags], cwd=copy, check=True)
    shutil.copyfile(copy / "LICENSE", output / "LICENSE-PixieEditor.txt")


def prepare(work: Path, output: Path, editor: Path) -> Path:
    output.mkdir()
    bundle = output / "Pixie-OpenGL43-VM-Test"
    bundle.mkdir()
    sdk = bundle / "SDK"
    compose([work / "u-authoring", work / "u-shipping"], sdk, complete=False)
    catalog_file = sdk / "sdk.json"
    catalog = read_json(catalog_file)
    catalog["qualification"] = "Local OpenGL 4.3 VM test only. Release configuration; no publication."
    for entry in catalog["profiles"]:
        profile_file = sdk / entry["manifest"]
        profile = read_json(profile_file)
        profile["systemPrerequisites"] = ["Windows x64", "OpenGL 4.3 core driver"]
        write_json(profile_file, profile)
        entry["manifestSha256"] = digest(profile_file)
        if entry["profile"] == "shipping":
            shipping = profile
    catalog["files"] = inventory(sdk)
    write_json(catalog_file, catalog)
    pair_exporter(ENGINE, output, sdk, catalog, shipping)
    verify(sdk)

    source = ENGINE / "samples/light-lab"
    project = bundle / "Scene"
    project.mkdir()
    shutil.copytree(source / "images", project / "images")
    shutil.copyfile(source / "light-lab.pixie", project / "light-lab.pixie")
    scene = read_json(source / "scenes/main.scene")
    scene["entities"] = [entity for entity in scene["entities"] if "LightLabScenario" not in entity["components"]]
    (project / "scenes").mkdir()
    write_json(project / "scenes/main.scene", scene)
    subprocess.run([sdk / catalog["tools"]["exporter"]["path"], "--sdk", sdk,
                    "--work", output / "export-work", project, bundle / "Player"], cwd=output, check=True)

    publish_editor(editor, output / "editor-source", bundle / "Editor", sdk)
    for dependency in shipping["nativeDependencies"]:
        file = sdk / dependency["path"]
        if file.name.startswith(("msvcp", "vcruntime", "concrt")):
            target = bundle / "Editor" / file.name
            if not target.exists():
                shutil.copyfile(file, target)

    for filename in ("Run VM Graphics Test.cmd", "README.txt"):
        shutil.copyfile(Path(__file__).with_name(filename), bundle / filename)
    if any(bundle.rglob("opengl32.dll")):
        raise ValueError("The VM test must use the guest's OpenGL driver")

    files = {entry["path"]: (entry["size"], entry["sha256"]) for entry in inventory(bundle)}
    archive = output / f"{bundle.name}.zip"
    check_tree(bundle, files)
    archive_tree(bundle, files, archive)
    verify_and_relocate(archive, files, output / "relocated-test")
    return archive


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", required=True, type=Path, help="Directory containing the two native install units")
    parser.add_argument("--out", required=True, type=Path, help="New package output directory")
    parser.add_argument("--editor", required=True, type=Path, help="Standalone PixieEditor source directory")
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("Run from an x64 Visual Studio developer shell")
    archive = prepare(args.work.resolve(), args.out.resolve(), args.editor.resolve())
    print(f"{digest(archive)}  {archive}")


if __name__ == "__main__":
    main()
