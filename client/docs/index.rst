Welcome to HoloOcean's documentation!
==============================================
.. image:: images/inspect_plane.jpg

HoloOcean is a realistic underwater robotics simulator with multi-agent missions, various underwater sensors including a novel imaging sonar sensor 
implementation, easy installation, and simple use. It's a fork of `Holodeck <https://github.com/BYU-PCCL/holodeck>`_, a high-fidelity reinforcement learning simulator
built on Unreal Engine 4.

If you use HoloOcean in your research, please cite our conference publication with the following bibtex:

::
      
   @inproceedings{Potokar22icra,
      author = {E. Potokar and S. Ashford and M. Kaess and J. Mangelson},
      title = {Holo{O}cean: An Underwater Robotics Simulator},
      booktitle = {Proc. IEEE Intl. Conf. on Robotics and Automation, ICRA},
      address = {Philadelphia, PA, USA},
      month = may,
      year = {2022}
   }

.. toctree::
   :maxdepth: 2
   :caption: HoloOcean Documentation

   usage/installation
   usage/getting-started
   usage/usage
   packages/packages
   agents/agents
   develop/develop
   changelog/changelog

.. toctree::
   :maxdepth: 3
   :caption: API Documentation

   holoocean/index
   holoocean/agents
   holoocean/environments
   holoocean/spaces
   holoocean/commands
   holoocean/holooceanclient
   holoocean/packagemanager
   holoocean/sensors
   holoocean/lcm
   holoocean/shmem
   holoocean/util
   holoocean/exceptions
   holoocean/weather


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
