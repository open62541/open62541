using NUnit.Framework;

namespace Opc.Ua.Interop.Tests
{
    [TestFixture]
    [Explicit("Requires a released OPC Foundation .NET Standard package with WSS support")]
    [Category("ExperimentalWssInterop")]
    [NonParallelizable]
    public sealed class ExperimentalWssInteropClientTest : InteropClientTestBase
    {
        protected override string ServerUrlEnvironmentVariable =>
            "OPCUA_INTEROP_WSS_SERVER_URL";
        protected override string DefaultServerUrl => string.Empty;
        protected override bool RequireServerUrl => true;
    }
}
