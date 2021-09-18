Changelog
=========

.. Changelog Style Guide
  - Each release should have a New Features / Changes / Bug Fixes section.
  - Keep the first sentence of each point short and descriptive
  - The passive voice should be avoided
  - Try to make the first word a verb in past tense. Bug fixes should use
    "Fixed"
  - Add a link to the issue describing the change or the pull request that
    merged it at the end in parentheses
  - see https://github.com/BYU-PCCL/holodeck/wiki/Holodeck-Release-Notes-Template

HoloOcean 0.4.0
----------------
*9/17/2021*

First official release!

Highlights
~~~~~~~~~~
- New Ocean environment package.
- 2 new agents and 7 new sensors, along with updating of all previous sensors.
- Complete rebranding to HoloOcean.  

New Features
~~~~~~~~~~~~
- Added agents :class:`~holoocean.agents.HoveringAUV` and :class:`~holoocean.agents.TorpedoAUV`
- Added a plethora of new sensors, all with optional noise configurations

  - :class:`~holoocean.sensors.SonarSensor`
  - :class:`~holoocean.sensors.DVLSensor`
  - :class:`~holoocean.sensors.DepthSensor`
  - :class:`~holoocean.sensors.GPSSensor`
  - :class:`~holoocean.sensors.PoseSensor`
  - :class:`~holoocean.sensors.AcousticBeaconSensor`
  - :class:`~holoocean.sensors.OpticalModemSensor`
- New :ref:`Ocean <ocean>` package.
- Added frame rate capping option.
- Added ticks_per_sec and frames_per_sec to scenario config, see :ref:`configure-framerate`.

Changes
~~~~~~~
- Everything is now rebranded from Holodeck -> HoloOcean.

Bug Fixes
~~~~~~~~~
- Sensors now return values from their location, not the agent location.
- IMU now returns angular velocity instead of linear velocity.
- Various integer -> float changes in scenario loading.


Pre-HoloOcean
--------------
See `Holodeck changelog <https://holodeck.readthedocs.io/en/latest/changelog/changelog.html>`_