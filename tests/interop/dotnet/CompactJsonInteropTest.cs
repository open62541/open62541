/* ========================================================================
 * Compact JSON ExtensionObject interoperability with open62541.
 * ======================================================================*/

using NUnit.Framework;
using Opc.Ua;

namespace Opc.Ua.Interop.Tests
{
    [TestFixture]
    [Category("JsonInterop")]
    [NonParallelizable]
    public sealed class CompactJsonInteropTest
    {
        private static ReadValueId CreateReadValueId() => new()
        {
            NodeId = VariableIds.Server_ServerStatus,
            AttributeId = Attributes.Value
        };

        private static void AssertReadValueId(ExtensionObject extensionObject)
        {
            Assert.That(extensionObject.Body, Is.TypeOf<ReadValueId>());
            var value = (ReadValueId)extensionObject.Body;
            Assert.That(value.NodeId, Is.EqualTo(VariableIds.Server_ServerStatus));
            Assert.That(value.AttributeId, Is.EqualTo(Attributes.Value));
        }

        [Test]
        public void BidirectionalCompactExtensionObjects()
        {
            string? cEncodedPath = Environment.GetEnvironmentVariable(
                "OPCUA_INTEROP_JSON_C_ENCODED");
            string? dotnetEncodedPath = Environment.GetEnvironmentVariable(
                "OPCUA_INTEROP_JSON_DOTNET_ENCODED");
            if (string.IsNullOrWhiteSpace(cEncodedPath) ||
                string.IsNullOrWhiteSpace(dotnetEncodedPath))
            {
                Assert.Ignore("Compact JSON interop vector paths are not configured.");
            }

            IServiceMessageContext context =
                new ServiceMessageContext(new InteropTelemetryContext());
            string cEncoded = File.ReadAllText(cEncodedPath!);
            using (var decoder = new JsonDecoder(cEncoded, context))
            {
                AssertReadValueId(decoder.ReadExtensionObject("ExtensionObject"));

                Variant variant = decoder.ReadVariant("Variant");
                Assert.That(variant.Value, Is.TypeOf<ExtensionObject>());
                AssertReadValueId((ExtensionObject)variant.Value);

                ExtensionObjectCollection values =
                    decoder.ReadExtensionObjectArray("ExtensionObjects");
                Assert.That(values, Has.Count.EqualTo(2));
                AssertReadValueId(values[0]);
                AssertReadValueId(values[1]);
            }

            var extensionObject = new ExtensionObject(CreateReadValueId());
            using var encoder = new JsonEncoder(context, JsonEncodingType.Compact);
            encoder.WriteExtensionObject("ExtensionObject", extensionObject);
            encoder.WriteVariant("Variant", new Variant(extensionObject));
            encoder.WriteExtensionObjectArray("ExtensionObjects",
                new ExtensionObject[] { extensionObject, extensionObject });
            File.WriteAllText(dotnetEncodedPath!, encoder.CloseAndReturnText());
        }
    }
}
