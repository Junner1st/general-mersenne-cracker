from setuptools import setup, Extension

mt_ext = Extension(
    name='mt19937',
    sources=['mt19937.c'],
    extra_compile_args=['-O3', '-march=native'],
)

bm_ext = Extension(
    name='bitmatrix',
    sources=['mt19937.c', 'bitmatrix.c'],
    extra_compile_args=['-O3', '-march=native'],
)

setup(
    name='cracker',
    ext_modules=[mt_ext, bm_ext],
)