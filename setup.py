from setuptools import setup, find_packages

with open("README.md") as f:
    readme = f.read()

setup(
    name="holoocean",
    version="0.1.0",
    description="Autonomous Underwater Vehicle Simulator",
    long_description=readme,
    long_description_content_type="text/markdown",
    author="Easton Potokar, Spencer Ashford, BYU FRoSt & PCCL Labs",
    author_email="contagon@byu.edu",
    url="https://bitbucket.org/frostlab/holoocean",
    packages=find_packages('src'),
    package_dir={'': 'src'},
    license='MIT License',
    python_requires=">=3.5",
    install_requires=[
        'posix_ipc >= 1.0.0; platform_system == "Linux"',
        'pywin32 >= 1.0; platform_system == "Windows"',
        'numpy'
    ],
)
