/* ========================================================================
 * OPC UA .NET HTTPS client interoperability tests against open62541.
 * ======================================================================*/

using System.Collections.Concurrent;
using Microsoft.Extensions.Logging;
using NUnit.Framework;
using Opc.Ua;
using Opc.Ua.Client;
using Opc.Ua.Configuration;

namespace Opc.Ua.Interop.Tests
{
    [TestFixture]
    [Category("HttpInterop")]
    [NonParallelizable]
    public sealed class HttpsInteropClientTest
    {
        private const string HttpsBinaryProfile =
            "http://opcfoundation.org/UA-Profile/Transport/https-uabinary";

        private ApplicationConfiguration _config = null!;
        private DefaultSessionFactory _sessionFactory = null!;
        private ITelemetryContext _telemetry = null!;
        private string _serverUrl = null!;
        private string _pkiRoot = null!;

        [OneTimeSetUp]
        public async Task OneTimeSetUp()
        {
            _serverUrl = Environment.GetEnvironmentVariable(
                "OPCUA_INTEROP_HTTPS_SERVER_URL") ?? string.Empty;
            if (string.IsNullOrWhiteSpace(_serverUrl))
            {
                Assert.Ignore(
                    "Set OPCUA_INTEROP_HTTPS_SERVER_URL to run HTTPS interoperability tests.");
            }

            _telemetry = new InteropTelemetryContext();
            _pkiRoot = Path.Combine(Path.GetTempPath(),
                "https_interop_pki_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_pkiRoot);

            var applicationCerts =
                ApplicationConfigurationBuilder.CreateDefaultApplicationCertificates(
                    "CN=HttpsInteropClient, O=open62541, DC=localhost",
                    CertificateStoreType.Directory,
                    _pkiRoot);

            _config = await new ApplicationInstance(_telemetry)
            {
                ApplicationName = "HttpsInteropClient",
                ApplicationType = ApplicationType.Client
            }
                .Build("urn:localhost:open62541:HttpsInteropClient",
                       "http://open62541.org/UA/HttpsInteropClient")
                .AsClient()
                .AddSecurityConfiguration(applicationCerts, _pkiRoot)
                .SetAutoAcceptUntrustedCertificates(true)
                .SetRejectSHA1SignedCertificates(false)
                .SetMinimumCertificateKeySize(0)
                .CreateAsync().ConfigureAwait(false);

            var application = new ApplicationInstance(_telemetry)
            {
                ApplicationName = "HttpsInteropClient",
                ApplicationType = ApplicationType.Client,
                ApplicationConfiguration = _config
            };
            await application.CheckApplicationInstanceCertificatesAsync(true)
                .ConfigureAwait(false);
            _sessionFactory = new DefaultSessionFactory(_telemetry);
        }

        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            if (!string.IsNullOrEmpty(_pkiRoot) && Directory.Exists(_pkiRoot))
                Directory.Delete(_pkiRoot, true);
        }

        private async Task<ConfiguredEndpoint> GetEndpointAsync()
        {
            EndpointDescription? endpoint = await CoreClientUtils.SelectEndpointAsync(
                _config, _serverUrl, useSecurity: false,
                telemetry: _telemetry, ct: CancellationToken.None)
                .ConfigureAwait(false);
            Assert.That(endpoint, Is.Not.Null);
            EndpointDescription selected = endpoint!;
            Assert.That(selected.TransportProfileUri, Is.EqualTo(HttpsBinaryProfile));
            Assert.That(selected.SecurityMode,
                Is.EqualTo(MessageSecurityMode.SignAndEncrypt));
            return new ConfiguredEndpoint(null, selected,
                EndpointConfiguration.Create(_config));
        }

        private Task<ISession> CreateSessionAsync(
            ConfiguredEndpoint endpoint, IUserIdentity identity)
        {
            return _sessionFactory.CreateAsync(
                _config, endpoint, false, false, "HTTPS interop", 30_000u,
                identity, default, CancellationToken.None);
        }

        private static async Task WaitUntilAsync(
            Func<bool> condition, TimeSpan timeout, string failure)
        {
            DateTime deadline = DateTime.UtcNow + timeout;
            while (!condition() && DateTime.UtcNow < deadline)
                await Task.Delay(50).ConfigureAwait(false);
            Assert.That(condition(), Is.True, failure);
        }

        private static async Task WriteAnswerAsync(ISession session, int value)
        {
            WriteResponse response = await session.WriteAsync(
                null,
                new WriteValue[]
                {
                    new()
                    {
                        NodeId = new NodeId("the.answer", 1),
                        AttributeId = Attributes.Value,
                        Value = new DataValue(new Variant(value))
                    }
                },
                CancellationToken.None).ConfigureAwait(false);
            Assert.That(response.Results, Has.Count.EqualTo(1));
            Assert.That(StatusCode.IsGood(response.Results[0]), Is.True);
        }

        private static async Task ExerciseSubscriptionAsync(ISession session)
        {
            using var subscription = new Subscription(session.DefaultSubscription)
            {
                PublishingInterval = 200,
                PublishingEnabled = true,
                KeepAliveCount = 2,
                LifetimeCount = 30,
                MaxNotificationsPerPublish = 10
            };
            var item = new MonitoredItem(subscription.DefaultItem)
            {
                StartNodeId = new NodeId("the.answer", 1),
                AttributeId = Attributes.Value,
                MonitoringMode = MonitoringMode.Reporting,
                SamplingInterval = 100,
                QueueSize = 10,
                DiscardOldest = true
            };
            var values = new ConcurrentQueue<int>();
            item.Notification += (monitoredItem, _) =>
            {
                foreach (DataValue value in monitoredItem.DequeueValues())
                {
                    if (StatusCode.IsGood(value.StatusCode) &&
                        value.WrappedValue.Value is int intValue)
                        values.Enqueue(intValue);
                }
            };
            int keepAlives = 0;
            subscription.FastKeepAliveCallback = (_, _) =>
                Interlocked.Increment(ref keepAlives);
            subscription.AddItem(item);
            Assert.That(session.AddSubscription(subscription), Is.True);
            await subscription.CreateAsync().ConfigureAwait(false);

            await WaitUntilAsync(() => !values.IsEmpty, TimeSpan.FromSeconds(10),
                "Initial monitored value was not delivered over HTTPS Publish");
            await WaitUntilAsync(() => Volatile.Read(ref keepAlives) > 0,
                TimeSpan.FromSeconds(10),
                "No subscription keepalive was delivered over HTTPS Publish");

            int beforeFirstWrite = values.Count;
            await WriteAnswerAsync(session, 1001).ConfigureAwait(false);
            await WaitUntilAsync(() => values.Count > beforeFirstWrite,
                TimeSpan.FromSeconds(10),
                "First post-keepalive data change was not delivered");

            int beforeSecondWrite = values.Count;
            await WriteAnswerAsync(session, 1002).ConfigureAwait(false);
            await WaitUntilAsync(() => values.Count > beforeSecondWrite,
                TimeSpan.FromSeconds(10),
                "Second data change was not delivered after Publish replenishment");

            await WriteAnswerAsync(session, 43).ConfigureAwait(false);
            await subscription.DeleteAsync(true).ConfigureAwait(false);
            Assert.That(values, Does.Contain(1001));
            Assert.That(values, Does.Contain(1002));
            TestContext.Out.WriteLine(
                $"HTTPS subscription: {values.Count} values, {keepAlives} keepalives");
        }

        [TestCase(false, TestName = "AnonymousSessionAndSubscription")]
        [TestCase(true, TestName = "UsernameSessionAndSubscription")]
        public async Task SessionAndSubscription(bool username)
        {
            ConfiguredEndpoint endpoint = await GetEndpointAsync().ConfigureAwait(false);
            IUserIdentity identity = username
                ? new UserIdentity("user1", "password"u8)
                : new UserIdentity();
            using ISession session = await CreateSessionAsync(endpoint, identity)
                .ConfigureAwait(false);
            Assert.That(session.Connected, Is.True);

            DataValue currentTime = await session.ReadValueAsync(
                VariableIds.Server_ServerStatus_CurrentTime,
                CancellationToken.None).ConfigureAwait(false);
            Assert.That(currentTime.StatusCode, Is.EqualTo(StatusCodes.Good));

            await ExerciseSubscriptionAsync(session).ConfigureAwait(false);
            await session.CloseAsync(CancellationToken.None).ConfigureAwait(false);
        }

        [Test]
        public async Task WrongPasswordIsRejected()
        {
            ConfiguredEndpoint endpoint = await GetEndpointAsync().ConfigureAwait(false);
            ServiceResultException exception = Assert.ThrowsAsync<ServiceResultException>(
                async () =>
                {
                    using ISession session = await CreateSessionAsync(endpoint,
                        new UserIdentity("user1", "wrong-password"u8))
                        .ConfigureAwait(false);
                })!;
            Assert.That(exception.StatusCode, Is.AnyOf(
                StatusCodes.BadUserAccessDenied,
                StatusCodes.BadIdentityTokenRejected,
                StatusCodes.BadIdentityTokenInvalid));
        }
    }
}
