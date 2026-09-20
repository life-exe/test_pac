"""Install dependencies for Debug and Release and configure the CMake project.

On Windows one build tree holds both configurations, so it is configured once.
On Linux a Makefile tree holds one configuration, so each gets its own.
"""

from common import CONFIGURATIONS, IS_WINDOWS, conan_install, configure

for configuration in CONFIGURATIONS:
    conan_install(configuration)
    if not IS_WINDOWS:
        configure(configuration)
if IS_WINDOWS:
    configure("Debug")
