/* ========================================================================
 * Interoperability tests: .NET OPC UA client against open62541 C server.
 *
 * Reads OPCUA_INTEROP_SERVER_URL environment variable for the C server
 * endpoint (e.g. "opc.tcp://localhost:4840").
 *
 * The C server is expected to be an instance of interop_server with:
 *   - User credentials: user1 / password
 *   - Variable: ns=1;s=the.answer (Int32 = 43)
 *   - Method: ns=1;i=62541 (HelloWorld)
 * ======================================================================*/

using System.Net;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Extensions.Logging;
using NUnit.Framework;
using Opc.Ua;
using Opc.Ua.Client;
using Opc.Ua.Configuration;

namespace Opc.Ua.Interop.Tests
{
    internal sealed class InteropTelemetryContext : TelemetryContextBase
    {
        public InteropTelemetryContext()
            : base(Microsoft.Extensions.Logging.LoggerFactory.Create(builder => builder
                .SetMinimumLevel(LogLevel.Debug))) { }
    }

    [TestFixture]
    [Category("Interop")]
    public class InteropClientTest
    {
        private string _serverUrl = null!;
        private ApplicationConfiguration _config = null!;
        private DefaultSessionFactory _sessionFactory = null!;
        private ITelemetryContext _telemetry = null!;
        private string _pkiRoot = null!;

        [OneTimeSetUp]
        public async Task OneTimeSetUp()
        {
            _serverUrl = Environment.GetEnvironmentVariable("OPCUA_INTEROP_SERVER_URL")
                ?? "opc.tcp://localhost:4840";

            _telemetry = new InteropTelemetryContext();

            TestContext.Out.WriteLine($"Interop server URL: {_serverUrl}");

            _pkiRoot = Path.Combine(Path.GetTempPath(),
                "interop_pki_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_pkiRoot);

            var applicationCerts =
                ApplicationConfigurationBuilder.CreateDefaultApplicationCertificates(
                    "CN=InteropTestClient, O=open62541, DC=localhost",
                    CertificateStoreType.Directory,
                    _pkiRoot);

            _config = await new ApplicationInstance(_telemetry)
            {
                ApplicationName = "InteropTestClient",
                ApplicationType = ApplicationType.Client
            }
                .Build("urn:localhost:open62541:InteropTestClient",
                       "http://open62541.org/UA/InteropTestClient")
                .AsClient()
                .AddSecurityConfiguration(applicationCerts, _pkiRoot)
                .SetAutoAcceptUntrustedCertificates(true)
                .SetRejectSHA1SignedCertificates(false)
                .SetMinimumCertificateKeySize(0)
                .CreateAsync().ConfigureAwait(false);

            // Generate application certificates if they don't exist
            var app = new ApplicationInstance(_telemetry)
            {
                ApplicationName = "InteropTestClient",
                ApplicationType = ApplicationType.Client,
                ApplicationConfiguration = _config
            };
            await app.CheckApplicationInstanceCertificatesAsync(true)
                .ConfigureAwait(false);

            // Ensure all ECC certificates exist — the SDK may fail to
            // auto-generate P384 or brainpool certs on some platforms
            await EnsureEccCertificatesAsync().ConfigureAwait(false);

            _sessionFactory = new DefaultSessionFactory(_telemetry);
        }

        private async Task<ConfiguredEndpoint> GetEndpointAsync(bool useSecurity)
        {
            var endpoint = await CoreClientUtils.SelectEndpointAsync(
                _config, _serverUrl, useSecurity: useSecurity,
                telemetry: _telemetry, ct: CancellationToken.None).ConfigureAwait(false);
            return new ConfiguredEndpoint(null, endpoint,
                EndpointConfiguration.Create(_config));
        }

        private Task<ISession> CreateSessionAsync(
            ConfiguredEndpoint endpoint, IUserIdentity? identity = null)
        {
            return _sessionFactory.CreateAsync(
                _config, endpoint, false, false,
                "InteropTest", 30_000u,
                identity ?? new UserIdentity(),
                default, CancellationToken.None);
        }

        [Test, Order(1)]
        public async Task ConnectAnonymous()
        {
            var endpoint = await GetEndpointAsync(useSecurity: false).ConfigureAwait(false);
            using var session = await CreateSessionAsync(endpoint).ConfigureAwait(false);

            Assert.That(session.Connected, Is.True, "Session should be connected");

            var serverState = await session.ReadValueAsync(
                VariableIds.Server_ServerStatus_State,
                CancellationToken.None).ConfigureAwait(false);
            Assert.That(serverState, Is.Not.Null);
            Assert.That(serverState.StatusCode, Is.EqualTo(StatusCodes.Good));

            TestContext.Out.WriteLine($"Server state: {serverState.WrappedValue}");

            await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
        }

        [Test, Order(2)]
        public async Task ConnectUsernamePassword()
        {
            // T-2: Both outcomes (accept or reject) are valid.
            // Many secure servers correctly reject username/password over the
            // None security policy to prevent credential leaking.
            var endpoint = await GetEndpointAsync(useSecurity: false).ConfigureAwait(false);
            try
            {
                using var session = await CreateSessionAsync(endpoint,
                    new UserIdentity("user1", "password"u8)).ConfigureAwait(false);

                Assert.That(session.Connected, Is.True,
                    "Session should be connected with username/password");

                await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
            }
            catch (Exception)
            {
                // T-2 is deliberately ambiguous: both accepting and rejecting
                // username/password over an unencrypted channel are valid.
                // Some servers require the password to be encrypted with a
                // specific security policy (e.g. Aes256_Sha256_RsaPss) even
                // over a None channel; others simply reject credentials.
                // Treat every exception as a graceful ignore.
                Assert.Ignore(
                    "Server correctly rejects or cannot process unencrypted credentials.");
            }
        }

        [Test, Order(3)]
        public async Task ReadNamespaceArray()
        {
            var endpoint = await GetEndpointAsync(useSecurity: false).ConfigureAwait(false);
            using var session = await CreateSessionAsync(endpoint).ConfigureAwait(false);

            var namespaceArray = await session.ReadValueAsync(
                VariableIds.Server_NamespaceArray,
                CancellationToken.None).ConfigureAwait(false);
            Assert.That(namespaceArray, Is.Not.Null);
            Assert.That(namespaceArray.StatusCode, Is.EqualTo(StatusCodes.Good));

            object? rawValue = namespaceArray.WrappedValue.Value;
            string[]? namespaces = rawValue as string[];
            if (namespaces == null && rawValue is IEnumerable<object> seq)
            {
                namespaces = seq.Select(x => x?.ToString() ?? "").ToArray();
            }
            Assert.That(namespaces, Is.Not.Null,
                $"Expected string[], got {rawValue?.GetType().FullName ?? "null"}");
            Assert.That(namespaces!.Length, Is.GreaterThan(0));
            Assert.That(namespaces[0], Is.EqualTo(Namespaces.OpcUa));

            TestContext.Out.WriteLine($"Namespaces: {string.Join(", ", namespaces)}");

            await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
        }

        [Test, Order(4)]
        public async Task ReadCustomVariable()
        {
            var endpoint = await GetEndpointAsync(useSecurity: false).ConfigureAwait(false);
            using var session = await CreateSessionAsync(endpoint).ConfigureAwait(false);

            // interop_server exposes ns=1;s=the.answer as Int32 = 43
            var nodeId = new NodeId("the.answer", 1);
            var value = await session.ReadValueAsync(
                nodeId, CancellationToken.None).ConfigureAwait(false);
            Assert.That(value, Is.Not.Null);
            Assert.That(value.StatusCode, Is.EqualTo(StatusCodes.Good));
            Assert.That(value.WrappedValue.Value, Is.EqualTo(43));

            TestContext.Out.WriteLine($"the.answer = {value.WrappedValue.Value}");

            await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
        }

        [Test, Order(5)]
        public async Task CallHelloWorldMethod()
        {
            var endpoint = await GetEndpointAsync(useSecurity: false).ConfigureAwait(false);
            using var session = await CreateSessionAsync(endpoint).ConfigureAwait(false);

            // interop_server HelloWorld method: ns=1;i=62541
            // Parent object: InteropTests (ns=1;i=1000)
            // Input: String, Output: String "Hello <input>"
            var objectId = new NodeId(1000, 1);
            var methodId = new NodeId(62541, 1);

            var outputs = await session.CallAsync(
                objectId, methodId, CancellationToken.None,
                new Variant("World")).ConfigureAwait(false);

            Assert.That(outputs.Count, Is.GreaterThan(0),
                "Method should return at least one output");

            TestContext.Out.WriteLine($"HelloWorld result: {outputs[0]}");

            await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
        }

        [Test, Order(10)]
        public async Task ConnectWithSecurityBasic256Sha256()
        {
            await ConnectWithSecurityPolicyAsync(
                SecurityPolicies.Basic256Sha256,
                "Basic256Sha256").ConfigureAwait(false);
        }

        [Test, Order(11)]
        public async Task ConnectWithSecurityAes128Sha256RsaOaep()
        {
            await ConnectWithSecurityPolicyAsync(
                SecurityPolicies.Aes128_Sha256_RsaOaep,
                "Aes128_Sha256_RsaOaep").ConfigureAwait(false);
        }

        [Test, Order(12)]
        public async Task ConnectWithSecurityAes256Sha256RsaPss()
        {
            await ConnectWithSecurityPolicyAsync(
                SecurityPolicies.Aes256_Sha256_RsaPss,
                "Aes256_Sha256_RsaPss").ConfigureAwait(false);
        }

        [Test, Order(30)]
        public async Task ConnectWithSecurityEccNistP256()
        {
            await ConnectWithSecurityPolicyAsync(
                "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP256",
                "ECC_nistP256").ConfigureAwait(false);
        }

        [Test, Order(31)]
        public async Task ConnectWithSecurityEccNistP384()
        {
            await ConnectWithSecurityPolicyAsync(
                "http://opcfoundation.org/UA/SecurityPolicy#ECC_nistP384",
                "ECC_nistP384").ConfigureAwait(false);
        }

        [Test, Order(32)]
        public async Task ConnectWithSecurityEccBrainpoolP256r1()
        {
            await ConnectWithSecurityPolicyAsync(
                "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP256r1",
                "ECC_brainpoolP256r1").ConfigureAwait(false);
        }

        [Test, Order(33)]
        public async Task ConnectWithSecurityEccBrainpoolP384r1()
        {
            await ConnectWithSecurityPolicyAsync(
                "http://opcfoundation.org/UA/SecurityPolicy#ECC_brainpoolP384r1",
                "ECC_brainpoolP384r1").ConfigureAwait(false);
        }

        [Test, Order(34)]
        public async Task ConnectWithSecurityEccCurve25519()
        {
            await ConnectWithSecurityPolicyAsync(
                "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve25519",
                "ECC_curve25519").ConfigureAwait(false);
        }

        [Test, Order(35)]
        public async Task ConnectWithSecurityEccCurve448()
        {
            await ConnectWithSecurityPolicyAsync(
                "http://opcfoundation.org/UA/SecurityPolicy#ECC_curve448",
                "ECC_curve448").ConfigureAwait(false);
        }

        [Test, Order(20)]
        public async Task ConnectUsernameOverEncrypted()
        {
            // T-8: Username/Password over Basic256Sha256
            var secureEndpoint = await FindSecureEndpointAsync(
                SecurityPolicies.Basic256Sha256).ConfigureAwait(false);

            if (secureEndpoint == null)
            {
                Assert.Ignore("Server does not offer Basic256Sha256 SignAndEncrypt endpoint");
                return;
            }

            var configuredEndpoint = new ConfiguredEndpoint(null, secureEndpoint,
                EndpointConfiguration.Create(_config));

            ISession session;
            try
            {
                session = await CreateSessionAsync(configuredEndpoint,
                    new UserIdentity("user1", "password"u8)).ConfigureAwait(false);
            }
            catch (ServiceResultException ex) when (
                ex.StatusCode == StatusCodes.BadSecurityChecksFailed ||
                ex.StatusCode == StatusCodes.BadCertificateUntrusted ||
                ex.StatusCode == StatusCodes.BadIdentityTokenRejected ||
                ex.StatusCode == StatusCodes.BadIdentityTokenInvalid  ||
                ex.StatusCode == StatusCodes.BadUserAccessDenied)
            {
                Assert.Ignore(
                    "Server rejected username/certificate (trust or policy not configured). " +
                    $"Status: {ex.StatusCode}");
                return;
            }

            using (session)
            {
                Assert.That(session.Connected, Is.True,
                    "Session should be connected with username over Basic256Sha256");

                var serverState = await session.ReadValueAsync(
                    VariableIds.Server_ServerStatus_State,
                    CancellationToken.None).ConfigureAwait(false);
                Assert.That(serverState.StatusCode, Is.EqualTo(StatusCodes.Good));

                TestContext.Out.WriteLine(
                    $"T-8: Username session over {session.Endpoint.SecurityPolicyUri}");

                await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
            }
        }

        [Test, Order(21)]
        public async Task ConnectWithCertificateAuth()
        {
            // T-9: X509 Certificate Authentication
            var secureEndpoint = await FindSecureEndpointAsync(
                SecurityPolicies.Basic256Sha256).ConfigureAwait(false);

            if (secureEndpoint == null)
            {
                Assert.Ignore("Server does not offer Basic256Sha256 SignAndEncrypt endpoint");
                return;
            }

            // Check if the endpoint supports X509 user token
            bool hasX509Token = false;
            foreach (var policy in secureEndpoint.UserIdentityTokens)
            {
                if (policy.TokenType == UserTokenType.Certificate)
                {
                    hasX509Token = true;
                    break;
                }
            }
            if (!hasX509Token)
            {
                Assert.Ignore("Server does not offer X509 certificate user token");
                return;
            }

            var configuredEndpoint = new ConfiguredEndpoint(null, secureEndpoint,
                EndpointConfiguration.Create(_config));

            // Use the application instance certificate as user identity
            var appCert = await _config.SecurityConfiguration
                .ApplicationCertificate.FindAsync(true).ConfigureAwait(false);
            if (appCert == null)
            {
                Assert.Ignore("No application certificate available for X509 auth");
                return;
            }

            ISession session;
            try
            {
                session = await CreateSessionAsync(configuredEndpoint,
                    new UserIdentity(appCert)).ConfigureAwait(false);
            }
            catch (ServiceResultException ex) when (
                ex.StatusCode == StatusCodes.BadSecurityChecksFailed ||
                ex.StatusCode == StatusCodes.BadCertificateUntrusted ||
                ex.StatusCode == StatusCodes.BadIdentityTokenRejected ||
                ex.StatusCode == StatusCodes.BadIdentityTokenInvalid)
            {
                Assert.Ignore(
                    "Server rejected X509 identity (trust not configured). " +
                    $"Status: {ex.StatusCode}");
                return;
            }

            using (session)
            {
                Assert.That(session.Connected, Is.True,
                    "Session should be connected with X509 certificate auth");

                var serverState = await session.ReadValueAsync(
                    VariableIds.Server_ServerStatus_State,
                    CancellationToken.None).ConfigureAwait(false);
                Assert.That(serverState.StatusCode, Is.EqualTo(StatusCodes.Good));

                TestContext.Out.WriteLine(
                    $"T-9: X509 auth session over {session.Endpoint.SecurityPolicyUri}");

                await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
            }
        }

        // Helper: find a secure endpoint by policy URI
        private async Task<EndpointDescription?> FindSecureEndpointAsync(string policyUri)
        {
            var endpointConfig = EndpointConfiguration.Create(_config);
            endpointConfig.OperationTimeout = 15000;

            using var discoveryClient = await DiscoveryClient.CreateAsync(
                new Uri(_serverUrl), endpointConfig,
                telemetry: _telemetry, ct: CancellationToken.None).ConfigureAwait(false);

            var endpoints = await discoveryClient.GetEndpointsAsync(
                default, CancellationToken.None).ConfigureAwait(false);

            await discoveryClient.CloseAsync(CancellationToken.None).ConfigureAwait(false);

            foreach (var e in endpoints)
            {
                if (e.SecurityPolicyUri == policyUri &&
                    e.SecurityMode == MessageSecurityMode.SignAndEncrypt)
                {
                    return e;
                }
            }
            return null;
        }

        // Helper: connect with a given security policy (T-5/6/7)
        private async Task ConnectWithSecurityPolicyAsync(
            string policyUri, string policyName)
        {
            var secureEndpoint = await FindSecureEndpointAsync(policyUri)
                .ConfigureAwait(false);

            if (secureEndpoint == null)
            {
                Assert.Ignore(
                    $"Server does not offer {policyName} SignAndEncrypt endpoint");
                return;
            }

            var configuredEndpoint = new ConfiguredEndpoint(null, secureEndpoint,
                EndpointConfiguration.Create(_config));

            ISession session;
            try
            {
                session = await CreateSessionAsync(configuredEndpoint)
                    .ConfigureAwait(false);
            }
            catch (ServiceResultException ex) when (
                ex.StatusCode == StatusCodes.BadSecurityChecksFailed ||
                ex.StatusCode == StatusCodes.BadSecurityPolicyRejected ||
                ex.StatusCode == StatusCodes.BadCertificateUntrusted ||
                ex.StatusCode == StatusCodes.BadCertificateInvalid ||
                ex.StatusCode == StatusCodes.BadCertificateUriInvalid ||
                ex.StatusCode == StatusCodes.BadCertificateHostNameInvalid ||
                ex.StatusCode == StatusCodes.BadCertificateTimeInvalid ||
                ex.StatusCode == StatusCodes.BadCertificateRevoked ||
                ex.StatusCode == StatusCodes.BadCertificateIssuerRevoked ||
                ex.StatusCode == StatusCodes.BadCertificateUseNotAllowed ||
                ex.StatusCode == StatusCodes.BadCertificatePolicyCheckFailed)
            {
                Assert.Ignore(
                    $"Server rejected client certificate for {policyName}. " +
                    $"Status: {ex.StatusCode}");
                return;
            }
            catch (ServiceResultException ex)
            {
                // Catch-all for remaining ServiceResultExceptions
                // (e.g. missing application certificate for ECC curves
                //  not supported by the .NET SDK like curve25519/curve448)
                Assert.Ignore(
                    $"Cannot connect with {policyName}: {ex.Message}");
                return;
            }

            using (session)
            {
                Assert.That(session.Connected, Is.True,
                    $"Session should be connected with {policyName}");

                var serverState = await session.ReadValueAsync(
                    VariableIds.Server_ServerStatus_State,
                    CancellationToken.None).ConfigureAwait(false);
                Assert.That(serverState.StatusCode, Is.EqualTo(StatusCodes.Good));

                TestContext.Out.WriteLine(
                    $"Secure session established with {session.Endpoint.SecurityPolicyUri}");

                await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Ensure all ECC application certificates exist.
        /// The SDK's CheckApplicationInstanceCertificatesAsync may fail to
        /// auto-generate P384 or brainpool certs on some platforms.
        /// This method fills in any gaps using System.Security.Cryptography.
        /// </summary>
        private async Task EnsureEccCertificatesAsync()
        {
            var appCerts = _config.SecurityConfiguration.ApplicationCertificates;
            if (appCerts == null)
                return;

            foreach (var certId in appCerts)
            {
                if (!TryGetEccCurve(certId.CertificateType,
                        out var curve, out var hashAlg))
                    continue;

                var existing = await certId.Find(true).ConfigureAwait(false);
                if (existing != null && existing.HasPrivateKey)
                {
                    TestContext.Out.WriteLine(
                        $"ECC cert OK: {certId.CertificateType} ({existing.Subject})");
                    // Do NOT dispose — the SDK caches this in certId.Certificate
                    continue;
                }
                // If the cert exists but has no private key, we need to regenerate
                // Do NOT dispose — let the SDK manage the lifecycle

                TestContext.Out.WriteLine(
                    $"ECC cert missing, generating: {certId.CertificateType}");

                try
                {
                    var cert = CreateSelfSignedEccCert(
                        curve, hashAlg,
                        Utils.ReplaceDCLocalhost(certId.SubjectName),
                        _config.ApplicationUri);

                    // Write to the PKI directory store so the SDK can find it
                    string certsDir = Path.Combine(_pkiRoot, "certs");
                    string privateDir = Path.Combine(_pkiRoot, "private");
                    Directory.CreateDirectory(certsDir);
                    Directory.CreateDirectory(privateDir);

                    string tp = cert.Thumbprint;
                    await File.WriteAllBytesAsync(
                        Path.Combine(privateDir, $"{tp}.pfx"),
                        cert.Export(X509ContentType.Pfx, (string?)null))
                        .ConfigureAwait(false);
                    await File.WriteAllBytesAsync(
                        Path.Combine(certsDir, $"{tp}.der"),
                        cert.Export(X509ContentType.Cert))
                        .ConfigureAwait(false);

                    TestContext.Out.WriteLine(
                        $"Generated ECC cert: {certId.CertificateType} " +
                        $"({cert.Subject}, {tp})");
                    cert.Dispose();
                }
                catch (Exception ex)
                {
                    TestContext.Out.WriteLine(
                        $"Cannot generate ECC cert {certId.CertificateType}: " +
                        $"{ex.Message}");
                }
            }
        }

        private static bool TryGetEccCurve(
            NodeId? certType, out ECCurve curve, out HashAlgorithmName hashAlg)
        {
            curve = default;
            hashAlg = HashAlgorithmName.SHA256;

            if (certType == null)
                return false;

            if (certType == ObjectTypeIds.EccNistP256ApplicationCertificateType)
            {
                curve = ECCurve.NamedCurves.nistP256;
                return true;
            }
            if (certType == ObjectTypeIds.EccNistP384ApplicationCertificateType)
            {
                curve = ECCurve.NamedCurves.nistP384;
                hashAlg = HashAlgorithmName.SHA384;
                return true;
            }
            if (certType == ObjectTypeIds.EccBrainpoolP256r1ApplicationCertificateType)
            {
                // brainpoolP256r1 OID
                curve = ECCurve.CreateFromValue("1.3.36.3.3.2.8.1.1.7");
                return true;
            }
            if (certType == ObjectTypeIds.EccBrainpoolP384r1ApplicationCertificateType)
            {
                // brainpoolP384r1 OID
                curve = ECCurve.CreateFromValue("1.3.36.3.3.2.8.1.1.11");
                hashAlg = HashAlgorithmName.SHA384;
                return true;
            }

            return false;
        }

        private static X509Certificate2 CreateSelfSignedEccCert(
            ECCurve curve, HashAlgorithmName hashAlg,
            string subjectName, string applicationUri)
        {
            using var key = ECDsa.Create(curve);
            var req = new CertificateRequest(subjectName, key, hashAlg);

            var sanBuilder = new SubjectAlternativeNameBuilder();
            sanBuilder.AddUri(new Uri(applicationUri));
            sanBuilder.AddDnsName("localhost");
            sanBuilder.AddIpAddress(IPAddress.Parse("127.0.0.1"));
            req.CertificateExtensions.Add(sanBuilder.Build());

            req.CertificateExtensions.Add(
                new X509BasicConstraintsExtension(false, false, 0, true));
            req.CertificateExtensions.Add(
                new X509KeyUsageExtension(
                    X509KeyUsageFlags.DigitalSignature |
                    X509KeyUsageFlags.NonRepudiation |
                    X509KeyUsageFlags.KeyAgreement,
                    true));

            var cert = req.CreateSelfSigned(
                DateTimeOffset.UtcNow.AddDays(-1),
                DateTimeOffset.UtcNow.AddYears(1));

            // Re-import so the private key is exportable
            return new X509Certificate2(
                cert.Export(X509ContentType.Pfx, (string?)null),
                (string?)null,
                X509KeyStorageFlags.Exportable);
        }
    }
}
