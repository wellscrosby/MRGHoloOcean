Package Structure
=================

A holoocean package is a ``.zip`` file containing a build of `holoocean-engine`_
that contains worlds and :ref:`scenarios` for those worlds.

.. _`holoocean-engine`: https://bitbucket.org/frostlab/holoocean-engine

A package file is platform specific, since it contains a compiled binary of
HoloOcean.

.. _`package-contents`:

Package Contents
----------------

The ``.zip`` file must contain the following elements

1. A build of `holoocean-engine`_

2. A ``config.json`` file that defines the worlds present in the package

3. Scenario configs for those worlds

.. _`package-structure`:

Package Structure
-----------------

The package.zip contains a ``config.json`` file at the root of the archive, as
well as all of the scenarios for every world included in the package. The
scenario files must follow the format ``{WorldName}-{ScenarioName}.json``.

::

   +package.zip
   +-- config.json
   +-- WorldName-ScenarioName.json
   +-- LinuxNoEditor
       + UE4 build output

config.json
-----------

This configuration file contains the package-level configuration. Below is 
the format the config file is expected to follow:

``config.json``:

.. code-block:: json

   {
      "name": "{package_name}",
      "platform": "{Linux | Windows}",
      "version": "{package_version}",
      "path" : "{path to binary within the archive}",
      "worlds": [
         {
            "name": "{world_name}",
            "pre_start_steps": 2,
            "env_min": [-10, -10, -10],
            "env_max": [10, 10, 10]
         }
      ]
   }

The ``"pre_start_steps"`` attribute for a world defines how many ticks should 
occur before starting the simulation, to work around world idiosyncrasies.

The ``env_min``/``env_max`` attributes are used to set the upper/lower bounds of the environment,
used when an octree is made for a sonar sensor.