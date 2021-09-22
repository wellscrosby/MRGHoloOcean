.. _installation:

============
Installation
============

HoloOcean is installed in two portions: a client python library (``holoocean``)
is installed first, which then downloads world packages. The python portion is
very small, while the world packages ("binaries") can be several gigabytes.


Requirements
============

- >= Python 3.6
- Several gigabytes of storage
- pip3
- Linux or Windows 64bit
- Preferably a competent GPU
- For Linux: OpenGL 3+

Stable Installation
=====================

The easiest installation is via the pypi repository, done as,
::

   pip install holoocean

And then to install the binary, simply run

::

   import holoocean
   holoocean.install("Ocean")


Or as a single console command,

::

   python -c `import holoocean; holoocean.install("Ocean")`

.. note::
   There is a bug on Windows with the package ``pywin32`` that occurs occasionally. If you see 
   "ImportError: DLL load failed while importing win32event: The specified module could not be found.",
   it can be fixed by running ``pip install pywin32==225``

Development Installation
==========================

To use the latest version of HoloOcean, you can install and use HoloOcean simply
by cloning the `bitbucket.org/frostlab/holoocean`_, and ensuring it is on your
``sys.path``.

.. _`bitbucket.org/frostlab/holoocean`: https://bitbucket.org/frostlab/holoocean/

The ``master`` branch is kept in sync with the pip repository, the ``develop``
branch is the bleeding edge of development.

To install the develop branch, simply run

::

   git clone https://bitbucket.org/frostlab/holoocean/
   cd holoocean
   git checkout develop
   pip install .


Then to install the most recent version of the oceans package, run the python command 

::

   import holoocean
   holoocean.install("Ocean", branch="develop")


Or as a single console command,

::

   python -c `import holoocean; holoocean.install("Ocean", branch="develop")`


.. _docker:

Note you can replace "develop" with whichever branch of HoloOcean-Engine you'd like to install.

Docker Installation
===================

HoloOcean's docker image is only supported on Linux hosts.

You will need ``nvidia-docker`` installed.

The repository on DockerHub is `frostlab/holoocean`_.

Currently the following tags are availible:

- ``base`` : base image without any worlds
- ``ocean`` : comes with the Ocean package preinstalled
- ``all/latest`` : comes with the all packages pre-installed

.. _`frostlab/holoocean`: https://hub.docker.com/r/frostlab/holoocean

This is an example command to start a holodeck container

``nvidia-docker run --rm -it --name holoocean frostlab/holoocean:ocean``

.. note::
   HoloOcean cannot be run with root privileges, so the user ``holooceanuser`` with
   no password is provided in the docker image.

Managing World Packages
=======================

The ``holodeck`` python package includes a :ref:`packagemanager` that is used
to download and install world packages. Below are some example usages, but see
:ref:`packagemanager` for complete documentation.

Install a Package Automatically
-------------------------------
::

   >>> from holoocean import packagemanager
   >>> packagemanager.installed_packages()
   []
   >>> packagemanager.available_packages()
   ['Ocean']
   >>> packagemanager.install("Ocean")
   Installing Ocean ver. 0.1.0 from https://robots.et.byu.edu/holo/Ocean/v0.1.0/Linux.zip
   File size: 1.55 GB
   |████████████████████████| 100%
   Unpacking worlds...
   Finished.
   >>> packagemanager.installed_packages()
   ['Ocean']

Installation Location
---------------------

By default, HoloOcean will install packages local to your user profile. See
:ref:`package-locations` for more information.

Manually Installing a Package
-----------------------------

To manually install a package, you will be provided a ``.zip`` file.
Extract it into the ``worlds`` folder in your HoloOcean installation location 
(see :ref:`package-locations`)

.. note::

   Ensure that the file structure is as follows:

   ::

      + worlds
      +-- YourManuallyInstalledPackage
      |   +-- config.json
      |    +-- etc...
      +-- AnotherPackage
      |   +-- config.json
      |   +-- etc...

   Not

   ::

      + worlds
      +-- YourManuallyInstalledPackage
      |   +-- YourManuallyInstalledPackage
      |       +-- config.json
      |   +-- etc...
      +-- AnotherPackage
      |   +-- config.json
      |   +-- etc...

Print Information
-----------------

There are several convenience functions provided to allow packages, worlds,
and scenarios to be easily inspected.

::

   >>> packagemanager.package_info("Ocean")
   Package: Ocean
      Platform: Linux
      Version: 0.1.0
      Path: LinuxNoEditor/Holodeck/Binaries/Linux/Holodeck
      Worlds:
      Rooms
            Scenarios:
            Rooms-DataGen:
               Agents:
                  Name: turtle0
                  Type: TurtleAgent
                  Sensors:
                     LocationSensor
                        lcm_channel: POSITION
                     RotationSensor
                        lcm_channel: ROTATION
                     RangeFinderSensor
                        lcm_channel: LIDAR
                        configuration
                           LaserCount: 64
                           LaserMaxDistance: 20
                           LaserAngle: 0
                           LaserDebug: True
            Rooms-IEKF:
               Agents:
                  Name: uav0
                  Type: UavAgent
                  Sensors:
                     PoseSensor
                     VelocitySensor
                     IMUSensor
      SimpleUnderwater
            Scenarios:
            SimpleUnderwater-AUV:
               Agents:
                  Name: auv0
                  Type: HoveringAUV
                  Sensors:
                     PoseSensor
                        socket: IMUSocket
                     VelocitySensor
                        socket: IMUSocket
                     IMUSensor
                        socket: IMUSocket
                     DVLSensor
                        socket: DVLSocket


You can also look for information for a specific world or scenario

::

   packagemanager.world_info("SimpleUnderwater")
   packagemanager.scenario_info("Rooms-DataGen")
