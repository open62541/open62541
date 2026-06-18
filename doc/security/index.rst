.. _security:
.. _security-overview:

Security
========

OPC UA security rests on two layers: the *SecureChannel* protects the wire
between client and server, and the *Session* authenticates the user on top of
an open channel.  open62541 ships with a complete, pluggable security stack
covering both.

This section documents the parts you actually configure and operate:

.. toctree::

   policies
   backends
   authentication
   certificates
   ecc

- :doc:`policies` -- the standard OPC UA SecurityPolicies supported by
  open62541 and how to register them on a server.
- :doc:`backends` -- the underlying crypto library (mbedTLS, OpenSSL, or
  LibreSSL) and the build flags that select it.
- :doc:`authentication` -- the identity-token types used to authenticate
  Sessions (Anonymous, Username/Password, X.509, Issued Token).
- :doc:`certificates` -- X.509 certificate management: trust lists, the
  three CertificateGroup backends (AcceptAll, Memorystore, Filestore),
  and the ``UA_CreateCertificate`` helper.
- :doc:`ecc` -- ECC specifics: supported curves, certificate requirements,
  generation, and the related examples.

For the protocol-level overview (SecureChannel, SecurityMode, Endpoint
discovery) -- see :doc:`../core_concepts`.  This section focuses on the
configuration and operational side.

For the auto-generated plugin API, see
:doc:`../plugin_securitypolicy` and :doc:`../plugin_certificategroup`.
