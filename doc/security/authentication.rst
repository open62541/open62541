.. _security-authentication:

Authentication
==============

Authentication in OPC UA happens at the *Session* layer, on top of an
already-open SecureChannel.  The client presents an *IdentityToken* during
``ActivateSession``.  The server validates it through its *AccessControl*
plugin and either accepts or rejects the session.

Identity token types
--------------------

The standard defines four token types.  All four are accepted by
open62541's default access-control plugin and may be enabled in any
combination.

.. list-table::
   :header-rows: 1
   :widths: 26 28 46

   * - Token
     - Type constant
     - Typical use
   * - Anonymous
     - ``UA_AnonymousIdentityToken``
     - Public, read-only data, discovery.  Opt out for protected servers.
   * - Username / Password
     - ``UA_UserNameIdentityToken``
     - Simple human-user login.  Credentials checked against an
       in-server list or a custom callback.
   * - X.509 Certificate
     - ``UA_X509IdentityToken``
     - Mutual-TLS-style login.  The client proves possession of the
       private key matching a certificate the server trusts.
   * - Issued Token
     - ``UA_IssuedIdentityToken``
     - External STS / OAuth-style flows.  The server forwards the token
       to an authorization service.

Default server configuration
----------------------------

``UA_AccessControl_default()`` (in
``plugins/ua_accesscontrol_default.c``) wires up the standard validator.
It checks anonymous logins, looks up username/password pairs in a static
list, and (when configured) verifies X.509 chains through a
``UA_CertificateGroup``.  The actual session-validation routine is
``activateSession_default()`` in the same file.

Useful entry points in the public plugin header
``plugins/include/open62541/plugin/accesscontrol_default.h``:

- ``UA_AccessControl_default()`` -- construct a default access-control
  plugin with a username/password allow-list and an optional
  ``UA_UsernamePasswordLoginCallback`` for custom validation.
- ``verifyX509`` -- a ``UA_CertificateGroup`` used to validate X.509
  identity tokens.  See :ref:`security-certificates`.

For the protocol-level description of how authentication fits into
``CreateSession`` / ``ActivateSession``, see :doc:`../core_concepts`
("Session" section).
