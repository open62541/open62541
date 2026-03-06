.. _json_config:

JSON Configuration
==================

Client and Server Configuration from JSON Files
------------------------------------------------

open62541 supports configuration of both clients and servers from JSON5-formatted files when the library is built with JSON encoding support (``UA_ENABLE_JSON_ENCODING``). This feature allows for easy, declarative configuration without recompiling your application.

JSON5 is an extension of JSON that allows for more human-readable configuration files, including:

- Comments (both single-line ``//`` and multi-line ``/* */``)
- Trailing commas in arrays and objects
- Unquoted object keys
- Single-quoted strings

.. note::
   The JSON configuration feature requires the ``UA_ENABLE_JSON_ENCODING`` build option to be enabled. This option is enabled by default in most builds. See :ref:`build_options` for more information.

Client Configuration API
------------------------

The following two functions are available for configuring OPC UA clients from JSON files:

UA_Client_newFromFile
^^^^^^^^^^^^^^^^^^^^^

Creates a new client instance directly from a JSON5 configuration file.

.. code-block:: c

   UA_Client *
   UA_Client_newFromFile(const UA_ByteString jsonConfig);

**Parameters:**

- ``jsonConfig``: A ``UA_ByteString`` containing the JSON5 configuration

**Returns:**

- A pointer to the newly created ``UA_Client`` instance, or ``NULL`` if the configuration is invalid

**Example usage:**

.. code-block:: c

   #include <open62541/client.h>

   UA_ByteString config = loadFile("client_config.json5");
   UA_Client *client = UA_Client_newFromFile(config);

   if(client) {
       UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
       // ... use the client
       UA_Client_delete(client);
   }
   UA_ByteString_clear(&config);

UA_ClientConfig_loadFromFile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Loads configuration into an existing ``UA_ClientConfig`` structure.

.. code-block:: c

   UA_StatusCode
   UA_ClientConfig_loadFromFile(UA_ClientConfig *config, const UA_ByteString jsonConfig);

**Parameters:**

- ``config``: Pointer to a ``UA_ClientConfig`` structure. This configuration will be cleared before loading.
- ``jsonConfig``: A ``UA_ByteString`` containing the JSON5 configuration

**Returns:**

- ``UA_STATUSCODE_GOOD`` on success
- An error code if the configuration is invalid or memory allocation fails

**Example usage:**

.. code-block:: c

   #include <open62541/client.h>

   UA_ClientConfig config;
   UA_ByteString jsonConfig = loadFile("client_config.json5");

   UA_StatusCode retval = UA_ClientConfig_loadFromFile(&config, jsonConfig);
   if(retval == UA_STATUSCODE_GOOD) {
       UA_Client *client = UA_Client_new(&config);
       // ... use the client
       UA_Client_delete(client);
   }
   UA_ByteString_clear(&jsonConfig);

Server Configuration API
------------------------

Similarly, two functions are available for configuring OPC UA servers from JSON files:

UA_Server_newFromFile
^^^^^^^^^^^^^^^^^^^^^

Creates a new server instance directly from a JSON5 configuration file.

.. code-block:: c

   UA_Server *
   UA_Server_newFromFile(const UA_ByteString jsonConfig);

**Parameters:**

- ``jsonConfig``: A ``UA_ByteString`` containing the JSON5 configuration

**Returns:**

- A pointer to the newly created ``UA_Server`` instance, or ``NULL`` if the configuration is invalid

**Example usage:**

.. code-block:: c

   #include <open62541/server.h>

   UA_ByteString config = loadFile("server_config.json5");
   UA_Server *server = UA_Server_newFromFile(config);

   if(server) {
       UA_StatusCode retval = UA_Server_runUntilInterrupt(server);
       UA_Server_delete(server);
   }
   UA_ByteString_clear(&config);

UA_ServerConfig_loadFromFile
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Loads configuration into an existing ``UA_ServerConfig`` structure.

.. code-block:: c

   UA_StatusCode
   UA_ServerConfig_loadFromFile(UA_ServerConfig *config, const UA_ByteString jsonConfig);

**Parameters:**

- ``config``: Pointer to a ``UA_ServerConfig`` structure. This configuration will be cleared before loading.
- ``jsonConfig``: A ``UA_ByteString`` containing the JSON5 configuration

**Returns:**

- ``UA_STATUSCODE_GOOD`` on success
- An error code if the configuration is invalid or memory allocation fails

**Example usage:**

.. code-block:: c

   #include <open62541/server.h>

   UA_Server *server = UA_Server_new();
   UA_ServerConfig *config = UA_Server_getConfig(server);
   UA_ByteString jsonConfig = loadFile("server_config.json5");

   UA_StatusCode retval = UA_ServerConfig_loadFromFile(config, jsonConfig);
   if(retval == UA_STATUSCODE_GOOD) {
       UA_Server_runUntilInterrupt(server);
   }
   UA_Server_delete(server);
   UA_ByteString_clear(&jsonConfig);

Client Configuration Examples
------------------------------

Minimal Client Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The simplest client configuration requires only an endpoint URL and application description:

.. code-block:: json5

   // client_config_minimal.json5
   {
     endpointUrl: "opc.tcp://localhost:4840",

     applicationDescription: {
       applicationUri: "urn:open62541.client.application",
       applicationName: {
         text: "OPC UA Client"
       }
     }
   }

This minimal configuration is sufficient for connecting to an unsecured OPC UA server.

Complete Client Configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A more comprehensive client configuration with all commonly used options:

.. code-block:: json5

   // client_config.json5
   {
     // Connection timeout in milliseconds
     timeout: 1000,

     // Application description
     applicationDescription: {
       applicationUri: "urn:open62541.client.application",
       productUri: "urn:product.test.org",
       applicationName: {
         locale: "en-EN",
         text: "Test Application"
       },
     },

     // Server endpoint to connect to
     endpointUrl: "opc.tcp://localhost:4840",

     // Preferred locale IDs for the session
     sessionLocaleIds: ["en_US", "de_DE"],

     // User identity token settings
     userTokenPolicy: {
       policyId: "username",
       tokenType: "UserName",
       issuedTokenType: null,
       issuerEndpointUrl: null,
       securityPolicyUri: "http://opcfoundation.org/UA/SecurityPolicy#None"
     },
     userName: "user",              // Username for authentication
     password: "password",          // Password for authentication

     // Session and connection settings
     noSession: false,              // Set to true to connect without a session
     noReconnect: false,            // Set to true to disable automatic reconnection
     noNewSession: false,           // Set to true to prevent creating new sessions
     secureChannelLifeTime: 0,      // Secure channel lifetime (0 = default)
     requestedSessionTimeout: 1000, // Requested session timeout in milliseconds
     connectivityCheckInterval: 0,  // Connectivity check interval (0 = disabled)

     // Network settings
     tcpReuseAddr: true,            // Allow TCP address reuse

     // Application URI override
     applicationUri: "urn:open62541.server.application",

     // Security settings
     allowNonePolicyPassword: false, // Allow passwords with no security policy

     // Encryption settings (only if encryption is enabled)
     maxTrustListSize: 20,          // Maximum size of the trusted certificate list
     maxRejectedListSize: 20,       // Maximum size of the rejected certificate list

     // Subscription settings
     outStandingPublishRequests: 0  // Number of outstanding publish requests (0 = default)
   }

Server Configuration Example
-----------------------------

A complete server configuration example:

.. code-block:: json5

   // server_config.json5
   {
     // Application description
     applicationDescription: {
       applicationUri: "urn:open62541.server.application",
       productUri: "urn:product.server.org",
       applicationName: {
         locale: "en-US",
         text: "open62541-based OPC UA Server"
       },
       applicationType: "Server",
     },

     // Server endpoints configuration
     endpoints: [
       {
         securityMode: "None",
         securityPolicyUri: "http://opcfoundation.org/UA/SecurityPolicy#None"
       }
     ],

     // Network configuration
     serverUrls: ["opc.tcp://localhost:4840"],

     // Server limits
     maxSecureChannels: 10,
     maxSessions: 10,

     // Timeout settings (in milliseconds)
     secureChannelLifeTime: 600000,
     sessionTimeout: 600000,

     // Build information
     buildInfo: {
       productUri: "urn:product.server.org",
       manufacturerName: "open62541",
       productName: "open62541 OPC UA Server",
       softwareVersion: "1.0.0"
     }
   }

Loading Configuration Files
----------------------------

To load a configuration file from disk, you need a helper function to read the file into a ``UA_ByteString``. Here's a typical implementation:

.. code-block:: c

   #include <stdio.h>
   #include <stdlib.h>
   #include <open62541/types.h>

   UA_ByteString loadFile(const char *filename) {
       UA_ByteString fileContents = UA_BYTESTRING_NULL;

       FILE *fp = fopen(filename, "rb");
       if(!fp) {
           return fileContents;
       }

       // Get file size
       fseek(fp, 0, SEEK_END);
       long fileSize = ftell(fp);
       fseek(fp, 0, SEEK_SET);

       // Allocate buffer
       fileContents.data = (UA_Byte*)UA_malloc((size_t)fileSize);
       if(!fileContents.data) {
           fclose(fp);
           return fileContents;
       }

       // Read file
       fileContents.length = (size_t)fread(fileContents.data, 1, (size_t)fileSize, fp);
       fclose(fp);

       return fileContents;
   }

Complete Client Example
------------------------

Here is a complete example demonstrating how to use the JSON configuration API with a client:

.. include:: example_client_json_config.rst

This example:

1. Loads a JSON5 configuration file from the command line argument
2. Creates a client directly using ``UA_Client_newFromFile()``
3. Connects to the server specified in the configuration
4. Reads the current server time
5. Cleans up all resources

To run this example:

.. code-block:: bash

   # Compile the example
   gcc -o client_json_config client_json_config.c -lopen62541

   # Run with a configuration file
   ./client_json_config client_config.json5

Complete Server Example
------------------------

Here is a complete example demonstrating how to use the JSON configuration API with a server:

.. include:: example_server_json_config.rst

This example:

1. Loads a JSON5 configuration file from the command line argument
2. Creates a server directly using ``UA_Server_newFromFile()``
3. Runs the server until interrupted (Ctrl+C)
4. Cleans up all resources

To run this example:

.. code-block:: bash

   # Compile the example
   gcc -o server_json_config server_json_config.c -lopen62541

   # Run with a configuration file
   ./server_json_config server_config.json5

Configuration Reference Files
------------------------------

Complete example configuration files are provided in the ``examples/json_config`` directory:

- ``client_json_config_minimal.json5`` - Minimal client configuration
- ``client_json_config.json5`` - Complete client configuration with all options
- ``server_json_config.json5`` - Complete server configuration

These files can be used as templates for your own configurations.

Best Practices
--------------

1. **Start with minimal configuration**: Begin with a minimal configuration and add options as needed.

2. **Use comments**: JSON5 supports comments, so document your configuration choices directly in the file.

3. **Validate your JSON5**: Ensure your configuration file is valid JSON5 before deploying. Online validators are available.

4. **Handle errors**: Always check the return values of configuration functions and provide appropriate error messages.

5. **Use environment-specific files**: Maintain separate configuration files for development, testing, and production environments.

6. **Version control**: Keep your configuration files under version control along with your code.

7. **Security considerations**: Never store passwords or certificates directly in configuration files. Use secure credential storage mechanisms instead.

Troubleshooting
---------------

**Q: I get a compilation error about undefined references to** ``UA_Client_newFromFile``

**A:** Make sure your library was built with ``UA_ENABLE_JSON_ENCODING`` enabled. You can check this in your CMake configuration.

**Q: My configuration file is not being parsed correctly**

**A:** Verify that:

- The file is valid JSON5 syntax
- String values are properly quoted
- There are no trailing commas where they shouldn't be (though JSON5 allows them in most places)
- The file encoding is UTF-8

**Q: The client/server starts but doesn't use my configuration**

**A:** Ensure that:

- The configuration file is being loaded successfully (check return values)
- The ``UA_ByteString`` contains the actual file contents
- You're not overriding configuration options after loading the file

See Also
--------

- :ref:`build_options` - For information on enabling JSON encoding support
- :ref:`tutorials` - For more examples and tutorials
- The OPC UA specification for detailed information on configuration parameters
