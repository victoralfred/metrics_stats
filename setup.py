from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext
import pybind11

ext_modules = [
    Pybind11Extension(
        "sysstats",
        [
            "src/binding.cpp",
            "src/cpu_stats.cpp",
            "src/mem_stats.cpp",
            "src/disk_stats.cpp",
            "src/net_stats.cpp",
        ],
        include_dirs=[
            pybind11.get_include(),
            "include", "src"
        ],
        cxx_std=17,
    ),
]
setup(
    name="sysstats",
    version="0.1",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
