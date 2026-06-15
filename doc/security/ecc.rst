.. _security-ecc:

ECC Specifics
=============

ECC policies give equivalent or stronger security than the RSA-based
profiles with much shorter key lengths, which makes them well suited for
resource-constrained and modern deployments.  This page documents only
the ECC-specific configuration.  For the general picture read
:ref:`security-overview` first.

Supported curves
----------------

Each row corresponds to one SecurityPolicy.  The presence of a check
mark means that backend can register the policy given a matching
certificate and private key.

.. list-table::
   :header-rows: 1
   :widths: 36 32 16 16

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

Curve25519 and Curve448 require OpenSSL ≥ 3.0.  The NIST and Brainpool
curves require mbedTLS ≥ 3.0 or OpenSSL ≥ 3.0.  See
:ref:`security-backends` for the build flags.

Certificate requirements
------------------------

Each ECC policy requires its own certificate and private key, and the
key must match the curve.  A single certificate **cannot** be shared
across different curves.  RSA and ECC policies can coexist on the same
server.  Each carries its own key pair.

X.509 extensions to set when creating an ECC certificate:

- **Key Usage** -- ``keyAgreement`` is required (for ECDH key exchange).
  The recommended set for ECDSA curves is::

      digitalSignature, nonRepudiation, keyAgreement, keyCertSign

  RSA certificates use ``keyEncipherment`` instead.
- **SubjectAltName** -- must include at least ``URI:<ApplicationUri>``
  and ``DNS:<hostname>`` (or ``IP:...``).

Generating ECC certificates
---------------------------

There are three ways to obtain an ECC certificate for use with
open62541.

**1. ``UA_CreateCertificate`` (built-in, recommended for development)**

Pass ``key-type = "EC"`` and the desired ``ecc-curve`` value in the
key-value map.  Supported curves: ``prime256v1``, ``secp384r1``,
``brainpoolP256r1``, ``brainpoolP384r1``, ``ed25519``, ``ed448`` (last
two require OpenSSL).

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

**2. OpenSSL CLI (production, full control)**

ECDSA curves (P-256, P-384, Brainpool):

.. code-block:: bash

   openssl ecparam -name prime256v1 -genkey -noout -out server_nistP256.key.pem
   openssl req -new -x509 -key server_nistP256.key.pem \
       -out server_nistP256.cert.pem -days 3650 -sha256 \
       -subj "/C=DE/O=MyOrg/CN=OPC-UA-Server" \
       -addext "subjectAltName=URI:urn:my.app:server,DNS:localhost,IP:127.0.0.1" \
       -addext "basicConstraints=critical,CA:FALSE" \
       -addext "keyUsage=critical,digitalSignature,nonRepudiation,keyAgreement" \
       -addext "extendedKeyUsage=serverAuth,clientAuth"
   openssl x509  -in  server_nistP256.cert.pem -outform DER -out server_nistP256.cert.der
   openssl pkey  -in  server_nistP256.key.pem  -outform DER -out server_nistP256.key.der

Use ``-sha384`` and ``prime256v1`` replaced with ``secp384r1`` or
``brainpoolP256r1`` / ``brainpoolP384r1`` accordingly.

EdDSA curves (Curve25519, Curve448):

.. code-block:: bash

   openssl genpkey -algorithm ed25519 -out server_curve25519.key.pem
   openssl req -new -x509 -key server_curve25519.key.pem \
       -out server_curve25519.cert.pem -days 3650 \
       -subj "/C=DE/O=MyOrg/CN=OPC-UA-Server" \
       -addext "subjectAltName=URI:urn:my.app:server,DNS:localhost,IP:127.0.0.1" \
       -addext "basicConstraints=critical,CA:FALSE" \
       -addext "keyUsage=critical,digitalSignature,nonRepudiation,keyAgreement" \
       -addext "extendedKeyUsage=serverAuth,clientAuth"
   openssl x509 -in  server_curve25519.cert.pem -outform DER -out server_curve25519.cert.der
   openssl pkey -in  server_curve25519.key.pem  -outform DER -out server_curve25519.key.der

Replace ``ed25519`` with ``ed448`` for Curve448.

**3. ``tools/certs/create_self-signed.py`` (development helper)**

.. code-block:: bash

   # Single ECC certificate
   python3 tools/certs/create_self-signed.py /tmp/certs --ecc prime256v1

   # All supported ECC curves at once
   python3 tools/certs/create_self-signed.py /tmp/certs --ecc-all

The ``--ecc-all`` mode produces DER files named
``server_c_<curve>.cert.der`` / ``server_c_<curve>.key.der`` ready for
``--ecc-cert-dir``.
