.. _security-backends:

Crypto Backends
===============

open62541 does not implement cryptography itself.  It delegates to an
external library through a thin plugin layer.  The build option
``UA_ENABLE_ENCRYPTION`` selects which library is compiled in.  No security
policy from :ref:`security-policies` is available when this option is
``OFF``.

Backend selection
-----------------

.. list-table::
   :header-rows: 1
   :widths: 32 68

   * - CMake option
     - Effect
   * - ``UA_ENABLE_ENCRYPTION=OFF``
     - No encryption support.  Default.  Only the ``None`` policy is usable.
   * - ``UA_ENABLE_ENCRYPTION=MBEDTLS``
     - mbedTLS.  All RSA profiles + the four NIST/Brainpool ECC curves.
   * - ``UA_ENABLE_ENCRYPTION=OPENSSL``
     - OpenSSL.  All RSA profiles + every ECC curve, including
       Curve25519 and Curve448 (EdDSA).
   * - ``UA_ENABLE_ENCRYPTION=LIBRESSL``
     - LibreSSL.  Experimental.  Coverage is similar to OpenSSL.

The two boolean aliases ``UA_ENABLE_ENCRYPTION_OPENSSL`` and
``UA_ENABLE_ENCRYPTION_MBEDTLS`` are still accepted for backward
compatibility.  Prefer the unified ``UA_ENABLE_ENCRYPTION`` form in new
projects.

Hardware-backed key storage (TPM2)
----------------------------------

``UA_ENABLE_ENCRYPTION_TPM2=ON`` adds TPM2-backed key storage on top of an
already-selected ``UA_ENABLE_ENCRYPTION`` backend.  It is **not** a
standalone crypto library, so it must be combined with ``MBEDTLS`` or
``OPENSSL``.  When enabled, the private key used by the server lives on a
TPM2 device instead of in a DER file.  See the
``server_encryption_tpm_keystore`` and ``client_encryption_tpm_keystore``
examples, and the ``README_client_server_tpm_keystore.txt`` next to them
for the required software stack.

Library version requirements
----------------------------

- **mbedTLS ≥ 3.0** is required for the four NIST/Brainpool ECC curves.
  mbedTLS does not support Curve25519 / Curve448.
- **OpenSSL ≥ 3.0** is required for full ECC support including EdDSA
  curves.
- See :ref:`building` for the exact flag set and the
  ``UA_ENABLE_ENCRYPTION_TPM2`` option.

Registering policies
--------------------

The backend is selected at build time, not at runtime.  Once built with
``UA_ENABLE_ENCRYPTION=OPENSSL`` (or ``MBEDTLS``), adding a policy to a
server is one call per policy and the same call works with either backend:

.. code-block:: c

   UA_ServerConfig_addSecurityPolicyNone(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyEccNistP256(config, &cert, &key);
   /* ... or, in one shot: */
   UA_ServerConfig_addAllSecurityPolicies(config, &cert, &key);

``UA_ServerConfig_addAllSecurityPolicies()`` auto-detects the key type
(RSA vs EC) and registers all policies matching the key.  The
ECC-specific helpers and the full set of ``addSecurityPolicy*()`` calls
are listed in :ref:`security-ecc`.
