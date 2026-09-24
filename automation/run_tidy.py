"""Run clang-tidy over the project.

On Windows the analysis goes through MSBuild code analysis: the Visual Studio
generator writes no compile_commands.json, but MSBuild passes each file's real
compile flags. On Linux clang-tidy is called directly with the compilation
database that the Makefile generator exports.
Findings are errors (see .clang-tidy: WarningsAsErrors), so the run fails.

Usage:
    python automation/run_tidy.py
"""

from common import IS_WINDOWS, PROJECT_ROOT, build_dir, cmake_build, ensure_generated, relative, run

ensure_generated("Debug")

if IS_WINDOWS:
    run([
        "cmake", "--build", relative(build_dir("Debug")), "--config", "Debug",
        "--",
        "/p:RunCodeAnalysis=true",
        "/p:EnableClangTidyCodeAnalysis=true",
        "/p:EnableMicrosoftCodeAnalysis=false",
    ])
else:
    cmake_build(build_dir("Debug"), "Debug")  # generated headers and the database must exist
    sources = sorted((PROJECT_ROOT / "src").rglob("*.cpp"))
    run(["clang-tidy", "-p", relative(build_dir("Debug")), "--extra-arg=-Wno-ignored-gch", *(relative(s) for s in sources)]) 
