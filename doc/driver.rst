Drivers
=======

Drivers integrate external functionality with a server instance. Unlike the
core SDK, driver implementations can be architecture- and operating
system-specific. They may use platform facilities internally, but their
interaction with the server must go through the public server API.

Drivers are created separately and added to a server after the server has been
initialized. See the :ref:`drivers` for the
generic ``UA_Driver`` interface and lifecycle hooks.

.. toctree::

   driver_mdns
   driver_alarms_conditions
   driver_gds_receiver
