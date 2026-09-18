"""Build the Debug configuration and run all tests through CTest."""

from common import BUILD_DIR, build, run

build("Debug")
run(["ctest", "--test-dir", BUILD_DIR.name, "-C", "Debug", "--output-on-failure"])