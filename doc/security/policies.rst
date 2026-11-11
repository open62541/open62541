.. _security-policies:

Supported Security Policies
===========================

A *SecurityPolicy* is a named bundle of algorithms for asymmetric signing,
asymmetric encryption, symmetric encryption, key derivation, and message
signing.  OPC UA Part 7 defines the standard SecurityPolicies as *Profiles*
in the online Profile database at https://profiles.opcfoundation.org.  A
server advertises its supported policies through its *Endpoints* (returned
by ``GetEndpoints``).  The client picks one when opening a SecureChannel.

The tables below list every SecurityPolicy of the ``UACore 1.05`` Profile group
and show which crypto backend implements it. Adding a policy to a server is the
same call regardless of backend -- ``UA_ServerConfig_addSecurityPolicy*()``
(see :ref:`security-ecc` for the full call list).

Status and required policies
----------------------------

- **Current**: the Profile is released and not deprecated.
- **Deprecated**: the Profile database marks the Profile as deprecated.  Do
  not use it for new systems.

The conformance unit *Security Policy Required* asks every application that
supports security for at least SecurityPolicy [A] (``Aes128_Sha256_RsaOaep``)
and SecurityPolicy [B] (``Basic256Sha256``).  The conformance unit *Security
ECC Policy* asks for one of SecurityPolicy [ECC A] or [ECC B].  The Profile
database gives the label [ECC A] to ``ECC_nistP256_AesGcm`` and
``ECC_brainpoolP256r1_AesGcm``, and the label [ECC B] to
``ECC_curve25519_ChaChaPoly``.  Both conformance units also say that obsolete
SecurityPolicies shall not be enabled or usable without administrative
intervention.

Security level
--------------

Each ``UA_SecurityPolicy`` has a ``securityLevel`` that gives an estimate of
the strength of the security policy in relation to other security policies. The
default server configuration populates the security level of the endpoint
descriptions from the security level of the underlying security policy. Clients
usually prefer endpoints with the highest security level.


A security level of 0 is for ``None`` and for all deprecated policies.
``UA_ServerConfig_addAllSecureEndpoints`` creates no Endpoints for policies
with level 0.  PubSub SecurityPolicies have no security level.

SecureChannel SecurityPolicies
------------------------------

The table lists the SecurityPolicies that open62541 implements. A check mark in
the *Default* column means that ``UA_ServerConfig_addAllSecurityPolicies``,
``UA_ServerConfig_setDefaultWithSecurityPolicies`` and
``UA_ClientConfig_setDefaultEncryption`` add the policy if the certificate and
the private key match it.  The ``*Secure*`` variants of these server functions
do not add ``None``.

.. list-table::
   :header-rows: 1
   :widths: 24 12 6 8 8 8 8 26

   * - Policy
     - Status
     - Level
     - OpenSSL
     - LibreSSL
     - mbedTLS
     - Default
     - Notes
   * - ``None``
     - Current
     - 0
     - ✓
     - ✓
     - ✓
     - ✓
     - No signing, no encryption.  Also available without
       ``UA_ENABLE_ENCRYPTION``.
   * - ``Aes128_Sha256_RsaOaep``
     - Current [A]
     - 10
     - ✓
     - ✓
     - ✓
     - ✓
     - Required by *Security Policy Required*.
   * - ``Basic256Sha256``
     - Current [B]
     - 20
     - ✓
     - ✓
     - ✓
     - ✓
     - Required by *Security Policy Required*.
   * - ``Aes256_Sha256_RsaPss``
     - Current
     - 30
     - ✓
     - ✓
     - ✓
     - ✓
     - Strongest RSA policy.
   * - ``ECC_nistP256_AesGcm``
     - Current [ECC A]
     - 10
     - ✓
     - ✗
     - ✗
     - ✓
     - AES-128-GCM.  Satisfies *Security ECC Policy*.
   * - ``ECC_nistP256_ChaChaPoly``
     - Current
     - 10
     - ✓
     - ✗
     - ✗
     - ✓
     - ChaCha20-Poly1305.
   * - ``Basic128Rsa15``
     - Deprecated
     - 0
     - ✓
     - ✓
     - ✓
     - ✗
     - Known attacks.  The default functions add it only if the build
       defines ``UA_INCLUDE_INSECURE_POLICIES``.
   * - ``Basic256``
     - Deprecated
     - 0
     - ✓
     - ✓
     - ✓
     - ✗
     - Known attacks.  The default functions add it only if the build
       defines ``UA_INCLUDE_INSECURE_POLICIES``.
   * - ``ECC_nistP256``
     - Deprecated
     - 0
     - ✓
     - ✗
     - ✓
     - ✗
     - AES-128-CBC with HMAC-SHA256.  Replaced by ``ECC_nistP256_AesGcm``
       and ``ECC_nistP256_ChaChaPoly``.
   * - ``ECC_nistP384``
     - Deprecated
     - 0
     - ✓
     - ✗
     - ✓
     - ✗
     - AES-256-CBC with HMAC-SHA384.  Replaced by ``ECC_nistP384_AesGcm``
       and ``ECC_nistP384_ChaChaPoly`` (not implemented).
   * - ``ECC_brainpoolP256r1``
     - Deprecated
     - 0
     - ✓
     - ✗
     - ✓
     - ✗
     - AES-128-CBC with HMAC-SHA256.  Replaced by
       ``ECC_brainpoolP256r1_AesGcm`` and ``ECC_brainpoolP256r1_ChaChaPoly``
       (not implemented).
   * - ``ECC_brainpoolP384r1``
     - Deprecated
     - 0
     - ✓
     - ✗
     - ✓
     - ✗
     - AES-256-CBC with HMAC-SHA384.  Replaced by
       ``ECC_brainpoolP384r1_AesGcm`` and ``ECC_brainpoolP384r1_ChaChaPoly``
       (not implemented).
   * - ``ECC_curve25519``
     - Deprecated
     - 0
     - ✓
     - ✗
     - ✗
     - ✗
     - ChaCha20-Poly1305.  The Profile says that no application shall
       support it.  The current Curve25519 policies are
       ``ECC_curve25519_AesGcm`` and ``ECC_curve25519_ChaChaPoly`` (not
       implemented).
   * - ``ECC_curve448``
     - Deprecated
     - 0
     - ✓
     - ✗
     - ✗
     - ✗
     - ChaCha20-Poly1305.  The Profile says that no application shall
       support it.  The current Curve448 policies are
       ``ECC_curve448_AesGcm`` and ``ECC_curve448_ChaChaPoly`` (not
       implemented).

To use a deprecated policy, add it explicitly with its
``UA_ServerConfig_addSecurityPolicy*()`` function and create its Endpoints
with ``UA_ServerConfig_addEndpoint`` or ``UA_ServerConfig_addAllEndpoints``.
The constructors of the deprecated policies log a warning.

Not implemented
---------------

open62541 does not implement these SecurityPolicies of the ``UACore 1.05``
Profile group.

.. list-table::
   :header-rows: 1
   :widths: 36 20 44

   * - Policy
     - Status
     - Notes
   * - ``ECC_nistP384_AesGcm``
     - Current
     - Replaces ``ECC_nistP384``.
   * - ``ECC_nistP384_ChaChaPoly``
     - Current
     - Replaces ``ECC_nistP384``.
   * - ``ECC_brainpoolP256r1_AesGcm``
     - Current [ECC A]
     - Replaces ``ECC_brainpoolP256r1``.
   * - ``ECC_brainpoolP256r1_ChaChaPoly``
     - Current
     - Replaces ``ECC_brainpoolP256r1``.
   * - ``ECC_brainpoolP384r1_AesGcm``
     - Current
     - Replaces ``ECC_brainpoolP384r1``.
   * - ``ECC_brainpoolP384r1_ChaChaPoly``
     - Current
     - Replaces ``ECC_brainpoolP384r1``.
   * - ``ECC_curve25519_AesGcm``
     - Current
     -
   * - ``ECC_curve25519_ChaChaPoly``
     - Current [ECC B]
     -
   * - ``ECC_curve448_AesGcm``
     - Current
     -
   * - ``ECC_curve448_ChaChaPoly``
     - Current
     -
   * - ``RSA_DH_AesGcm``
     - Current
     - RSA certificates with Diffie-Hellman key agreement, AES-GCM.
   * - ``RSA_DH_ChaChaPoly``
     - Current
     - RSA certificates with Diffie-Hellman key agreement,
       ChaCha20-Poly1305.
   * - ``ECC_curve25519_ChaCha20Poly1305``
     - Deprecated
     -
   * - ``ECC_curve448_ChaCha20Poly1305``
     - Deprecated
     -

PubSub SecurityPolicies
-----------------------

.. list-table::
   :header-rows: 1
   :widths: 24 14 8 8 8 8 30

   * - Policy
     - Status
     - OpenSSL
     - LibreSSL
     - mbedTLS
     - TPM2
     - Notes
   * - ``PubSub-Aes128-CTR``
     - Current [A]
     - ✓
     - ✓
     - ✓
     - ✓
     - Required for UADP message signing (*PubSub Publisher UADP Security
       Signing*).
   * - ``PubSub-Aes256-CTR``
     - Current
     - ✓
     - ✓
     - ✓
     - ✓
     -

The TPM2 column needs ``UA_ENABLE_ENCRYPTION_TPM2=ON``.  The keys then stay
in the TPM and are used through PKCS#11.

Which of these you can actually enable depends on the :ref:`crypto backend
<security-backends>` and the certificate you load.  ECC-specifics -- the
curve support matrix, certificate requirements, and the
``UA_ServerConfig_addSecurityPolicy*Ecc*()`` helpers -- are documented in
:ref:`security-ecc`.
