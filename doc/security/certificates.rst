.. _security-certificates:

Certificates and Trust
======================

Every SecureChannel protected by anything other than the ``None`` policy
requires an X.509 certificate and matching private key on both sides.
Trust is managed by *CertificateGroup* plugins, which store the trusted
issuer list, the rejected-certificate list, and the CRLs used during
validation.

CertificateGroup backends
-------------------------

open62541 ships three CertificateGroup backends.  Pick one when you
configure your server.

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Backend
     - Notes
   * - ``UA_CertificateGroup_AcceptAll``
     - Trust every certificate.  Useful for tests and demos, **not for
       production**.
   * - ``UA_CertificateGroup_Memorystore``
     - Trust list kept in RAM.  Configurable size limits
       (``0:max-trust-listsize`` defaults to 64 KiB;
       ``0:max-rejected-listsize`` defaults to 100).
   * - ``UA_CertificateGroup_Filestore``
     - Trust list on disk under a PKI directory.  Default location is
       the current working directory, override with
       ``UA_CertificateGroup_Filestore``'s path argument.

Filestore PKI layout
--------------------

The Filestore backend reads and writes a fixed directory layout rooted
at the configured path:

.. code-block:: text

   pki/
   ├── ApplCerts/
   │   ├── issuer/{certs,crl}/
   │   ├── own/{certs,private}/
   │   ├── rejected/certs/
   │   └── trusted/{certs,crl}/
   └── UserTokenCerts/
       └── (same layout as ApplCerts)

Generate a certificate
----------------------

For development and self-signed certificates, ``UA_CreateCertificate``
(in ``plugins/include/open62541/plugin/create_certificate.h``) generates
a key pair and a self-signed certificate in one call.  RSA key sizes
2048 / 4096 are supported (4096 default).  ECC usage is documented in
:ref:`security-ecc`.

Certificate utility functions
-----------------------------

The ``UA_CertificateUtils_*`` helpers in
``include/open62541/plugin/certificategroup.h`` extract common
information from a DER certificate without exposing the underlying
crypto library:

- ``UA_CertificateUtils_verifyApplicationUri()`` -- check that the
  cert's SubjectAltName URI matches the application's
  ``ApplicationUri``.
- ``UA_CertificateUtils_getExpirationDate()`` -- X.509 ``notAfter``.
- ``UA_CertificateUtils_getSubjectName()`` -- X.509 Subject DN.
- ``UA_CertificateUtils_getThumbprint()`` -- SHA-1 thumbprint.
- ``UA_CertificateUtils_getKeySize()`` -- asymmetric key length in bits.
- ``UA_CertificateUtils_comparePublicKeys()`` -- compare public keys
  from a certificate and a CSR.

Revocation
----------

When a certificate fails trust validation, the standard OPC UA status
codes used by open62541 are:

- ``UA_STATUSCODE_BADCERTIFICATEREVOCATIONUNKNOWN``
- ``UA_STATUSCODE_BADCERTIFICATEISSUERREVOCATIONUNKNOWN``
- ``UA_STATUSCODE_BADCERTIFICATEISSUERREVOKED``
- ``UA_STATUSCODE_BADCERTIFICATEISSUERUSENOTALLOWED``

The Filestore backend's ``getCertificateCrls()`` operation returns the
CRLs relevant to a certificate, used by the verification step.

For the full plugin API, see :doc:`../plugin_certificategroup`.
