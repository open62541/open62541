.. _ecc-security:

ECC Security Policies
=====================

open62541 supports Elliptic Curve Cryptography (ECC) security policies as
defined in the OPC UA Security Profiles.  ECC policies provide equivalent or
stronger security than RSA-based policies with shorter key lengths, making them
suitable for resource-constrained and modern deployments.

Supported Curves
----------------

.. list-table::
   :header-rows: 1

   * - OPC UA Policy
     - Curve
     - OpenSSL
     - mbedTLS ≥ 3.0
   * - ``EccNistP256``
     - NIST P-256 (prime256v1)
     - ✓
     - ✓
   * - ``EccNistP384``
     - NIST P-384 (secp384r1)
     - ✓
     - ✓
   * - ``EccBrainpoolP256r1``
     - Brainpool P-256r1
     - ✓
     - ✓
   * - ``EccBrainpoolP384r1``
     - Brainpool P-384r1
     - ✓
     - ✓
   * - ``EccCurve25519``
     - Curve25519 (Ed25519)
     - ✓
     - ✗
   * - ``EccCurve448``
     - Curve448 (Ed448)
     - ✓
     - ✗

Prerequisites
-------------

- **OpenSSL ≥ 3.0** is recommended for full ECC support including
  Curve25519 and Curve448.
- **mbedTLS ≥ 3.0** supports NIST and Brainpool curves.  Curve25519 and
  Curve448 are not available with mbedTLS (OpenSSL only).
- Build with ``-DUA_ENABLE_ENCRYPTION=OPENSSL`` (or ``MBEDTLS``).

ECC Certificate Requirements
-----------------------------

Each ECC security policy requires its own certificate and private key that
matches the curve.  A single certificate **cannot** be shared across different
curves.  RSA and ECC policies can coexist on the same server, but each requires
its own key pair.

**Key Usage** (X.509 extension):

- ECC certificates **must** include ``keyAgreement`` (required for ECDH key
  exchange).  The recommended key usage for ECDSA curves is::

    digitalSignature, nonRepudiation, keyAgreement, keyCertSign

- RSA certificates use ``keyEncipherment`` instead of ``keyAgreement``.

**SubjectAltName** must include at minimum:

- ``URI:<ApplicationUri>`` — matching the server's ApplicationDescription
- ``DNS:<hostname>`` — for hostname validation

Generating ECC Certificates
----------------------------

Using ``UA_CreateCertificate`` (auto-generation)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The built-in certificate generator supports ECC when the ``key-type`` parameter
is set to ``"EC"``:

.. code-block:: c

   UA_KeyValueMap *kvm = UA_KeyValueMap_new();
   UA_UInt16 expiresIn = 365;
   UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                            &expiresIn, &UA_TYPES[UA_TYPES_UINT16]);
   UA_String keyType = UA_STRING_STATIC("EC");
   UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-type"),
                            &keyType, &UA_TYPES[UA_TYPES_STRING]);
   UA_String curve = UA_STRING_STATIC("prime256v1");
   UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "ecc-curve"),
                            &curve, &UA_TYPES[UA_TYPES_STRING]);

   UA_ByteString privateKey = UA_BYTESTRING_NULL;
   UA_ByteString certificate = UA_BYTESTRING_NULL;
   UA_CreateCertificate(UA_Log_Stdout, subject, lenSubject,
                        subjectAltName, lenSubjectAltName,
                        UA_CERTIFICATEFORMAT_DER, kvm,
                        &privateKey, &certificate);
   UA_KeyValueMap_delete(kvm);

Supported ``ecc-curve`` values: ``prime256v1``, ``secp384r1``,
``brainpoolP256r1``, ``brainpoolP384r1``, ``ed25519``, ``ed448``
(last two require OpenSSL).

Using OpenSSL command line
^^^^^^^^^^^^^^^^^^^^^^^^^^

For ECDSA curves (P-256, P-384, Brainpool):

.. code-block:: bash

   # Generate NIST P-256 key and self-signed certificate
   openssl ecparam -name prime256v1 -genkey -noout -out server_nistP256.key.pem
   openssl req -new -x509 -key server_nistP256.key.pem \
       -out server_nistP256.cert.pem -days 3650 -sha256 \
       -subj "/C=DE/O=MyOrg/CN=OPC-UA-Server" \
       -addext "subjectAltName=URI:urn:my.app:server,DNS:localhost,IP:127.0.0.1" \
       -addext "basicConstraints=critical,CA:FALSE" \
       -addext "keyUsage=critical,digitalSignature,nonRepudiation,keyAgreement" \
       -addext "extendedKeyUsage=serverAuth,clientAuth"
   # Convert to DER (required by open62541)
   openssl x509 -in server_nistP256.cert.pem -outform DER -out server_nistP256.cert.der
   openssl pkey -in server_nistP256.key.pem -outform DER -out server_nistP256.key.der

For EdDSA curves (Curve25519, Curve448):

.. code-block:: bash

   # Generate Ed25519 key and self-signed certificate
   openssl genpkey -algorithm ed25519 -out server_curve25519.key.pem
   openssl req -new -x509 -key server_curve25519.key.pem \
       -out server_curve25519.cert.pem -days 3650 \
       -subj "/C=DE/O=MyOrg/CN=OPC-UA-Server" \
       -addext "subjectAltName=URI:urn:my.app:server,DNS:localhost,IP:127.0.0.1" \
       -addext "basicConstraints=critical,CA:FALSE" \
       -addext "keyUsage=critical,digitalSignature,nonRepudiation,keyAgreement" \
       -addext "extendedKeyUsage=serverAuth,clientAuth"
   openssl x509 -in server_curve25519.cert.pem -outform DER -out server_curve25519.cert.der
   openssl pkey -in server_curve25519.key.pem -outform DER -out server_curve25519.key.der

Use ``-sha384`` for P-384 and brainpoolP384r1 curves.  Replace ``prime256v1``
with ``secp384r1``, ``brainpoolP256r1``, or ``brainpoolP384r1`` respectively.
Replace ``ed25519`` with ``ed448`` for Curve448.

Using the helper script ``create_self-signed.py``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``tools/certs/create_self-signed.py`` script can generate ECC certificates
for development and testing:

.. code-block:: bash

   # Generate a single ECC certificate (e.g. NIST P-256)
   python3 tools/certs/create_self-signed.py /tmp/certs --ecc prime256v1

   # Generate certificates for all supported ECC curves at once
   python3 tools/certs/create_self-signed.py /tmp/certs --ecc-all

The ``--ecc <curve>`` flag accepts: ``prime256v1``, ``secp384r1``,
``brainpoolP256r1``, ``brainpoolP384r1``, ``ed25519``, ``ed448``.
The ``--ecc-all`` flag generates certificates for all six curves.  Output files
are named ``server_c_<curve>.cert.der`` / ``server_c_<curve>.key.der``, ready
for use with the ``--ecc-cert-dir`` option of the ``server_encryption`` example.

Running the Encryption Server Example
--------------------------------------

The ``server_encryption`` example supports two modes for ECC:

**Auto-generated P-256 certificate** (no external files needed):

.. code-block:: bash

   ./server_encryption --ecc

This generates a NIST P-256 self-signed certificate at startup and configures
the server with ECC security policies matching that key.

**Multiple ECC curves from a certificate directory**:

.. code-block:: bash

   ./server_encryption server_rsa.cert.der server_rsa.key.der \
       --ecc-cert-dir /path/to/ecc_certs/

The directory must contain DER files named ``server_c_<curve>.cert.der`` and
``server_c_<curve>.key.der`` where ``<curve>`` is one of: ``nistP256``,
``nistP384``, ``brainpoolP256r1``, ``brainpoolP384r1``, ``curve25519``,
``curve448``.  Missing curves are silently skipped.  The RSA certificate
provided as positional argument enables the standard RSA policies alongside
the ECC policies.

Server API Usage
----------------

Adding individual ECC policies:

.. code-block:: c

   UA_ServerConfig_addSecurityPolicyEccNistP256(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyEccNistP384(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyEccBrainpoolP256r1(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyEccBrainpoolP384r1(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyEccCurve25519(config, &cert, &key);
   UA_ServerConfig_addSecurityPolicyEccCurve448(config, &cert, &key);
   /* Create endpoints for the added policies */
   UA_ServerConfig_addAllEndpoints(config);

Alternatively, ``UA_ServerConfig_addAllSecurityPolicies()`` auto-detects the key
type (RSA vs EC) and adds all matching policies.
