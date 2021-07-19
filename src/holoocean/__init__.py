"""Holodeck is a high fidelity simulator for reinforcement learning.
"""
# This is holodeck version, not holoocean
__version__ = '0.3.1'

from holoocean.holodeck import make
from holoocean.packagemanager import *

__all__ = ['agents', 'environments', 'exceptions', 'holodeck', 'lcm', 'make', 'packagemanager', 'sensors']
