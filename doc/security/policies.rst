.. _security-policies:

Supported Security Policies
===========================

A *SecurityPolicy* is a named bundle of algorithms for asymmetric signing,
asymmetric encryption, symmetric encryption, key derivation, and message
signing -- defined in Part 7 of the OPC UA standard.  A server advertises its
supported policies through its *Endpoints* (returned by ``GetEndpoints``).
The client picks one when opening a SecureChannel.

The table below lists every standard profile implemented in open62541.
Adding a policy to a server is the same call regardless of backend --
``UA_ServerConfig_addSecurityPolicy*()`` (see :ref:`security-ecc` for the
full call list).

.. list-table::
   :header-rows: 1
   :widths: 30 14 56

   * - Policy
     - Type
     - Notes
   * - ``None``
     - Universal
     - No signing, no encryption.  Required for unencrypted discovery.
   * - ``Basic128Rsa15``
     - RSA
     - Legacy.  RSA-1024 + AES-128.  Deprecated, do not use for new systems.
   * - ``Basic256``
     - RSA
     - RSA-2048 + AES-256.  Replaced by ``Basic256Sha256``.
   * - ``Basic256Sha256``
     - RSA
     - RSA-2048 + SHA-256 + AES-256.  The current RSA baseline.
   * - ``Aes128_Sha256_RsaOaep``
     - RSA
     - RSA-OAEP + AES-128 + SHA-256.  Modern RSA profile.
   * - ``Aes256_Sha256_RsaPss``
     - RSA
     - RSA-PSS + AES-256 + SHA-256.  Strongest RSA profile.
   * - ``ECC_nistP256``
     - ECC
     - NIST P-256 (prime256v1).  See :ref:`security-ecc`.
   * - ``ECC_nistP384``
     - ECC
     - NIST P-384 (secp384r1).  See :ref:`security-ecc`.
   * - ``ECC_brainpoolP256r1``
     - ECC
     - Brainpool P-256r1.  See :ref:`security-ecc`.
   * - ``ECC_brainpoolP384r1``
     - ECC
     - Brainpool P-384r1.  See :ref:`security-ecc`.
   * - ``ECC_curve25519``
     - ECC (EdDSA)
     - Curve25519 / Ed25519.  OpenSSL only.  See :ref:`security-ecc`.
   * - ``ECC_curve448``
     - ECC (EdDSA)
     - Curve448 / Ed448.  OpenSSL only.  See :ref:`security-ecc`.
   * - ``PubSub-Aes128-CTR``
     - PubSub
     - Symmetric-only, AES-128-CTR.  Used with PubSub message security.
   * - ``PubSub-Aes256-CTR``
     - PubSub
     - Symmetric-only, AES-256-CTR.  Used with PubSub message security.

Which of these you can actually enable depends on the :ref:`crypto backend
<security-backends>` and the certificate you load.  ECC-specifics -- the
curve support matrix, certificate requirements, and the
``UA_ServerConfig_addSecurityPolicy*Ecc*()`` helpers -- are documented in
:ref:`security-ecc`.
