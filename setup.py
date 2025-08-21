from setuptools import setup, Extension

module = Extension(
    name='mt19937',
    sources=['mt19937.c'],
    extra_compile_args=['-O3', '-march=native'],
)

setup(
    name='mt19937',
    ext_modules=[module],
)