Building for specific architectures
-----------------------------------

The open62541 library can be build for many operating systems and embedded systems.
This document shows a small excerpt of already tested architectures. Since the stack is only using the
C99 standard, there are many more supported architectures.

A full list of implemented architecture support can be found in the arch folder.

Windows, Linux, MacOS
^^^^^^^^^^^^^^^^^^^^^

These architectures are supported by default and are automatically chosen by CMake.

Have a look into the previous sections on how to do that.

freeRTOS + LwIP
^^^^^^^^^^^^^^^

Credits to `@cabralfortiss <https://github.com/cabralfortiss>`_

This documentation is based on the discussion of the PR https://github.com/open62541/open62541/pull/2511. If you have any doubts, please first check the discussion there.

This documentation assumes that you have a basic example using LwIP and freeRTOS that works fine, and you only want to add an OPC UA task to it.

There are two main ways to build open62541 for freeRTOS + LwIP:

- Select the cross compiler in CMake, set the flags needed for compilation (different for each microcontroller so it can be difficult) and then run make in the folder and the library should be generated. This method can be hard to do because you need to specify the include files and some other configurations.
- Generate the open6254.h and open6254.c files with the freeRTOSLWIP architecture and then put these files in your project in your IDE that you're using for compiling. This is the easiest way of doing it and the documentation only focus on this method.

In CMake, select freertosLWIP using the variable UA_ARCHITECTURE, enable amalgamation using the UA_ENABLE_AMALGAMATION variable and just select the native compilers. Then try to compile as always. The compilation will fail, but the open62541.h and open62541.c will be generated.

NOTE: If you are using the memory allocation functions from freeRTOS (pvPortMalloc and family) you will need also to set the variable UA_ARCH_FREERTOS_USE_OWN_MEMORY_FUNCTIONS to true. Many users had to implement pvPortCalloc and pvPortRealloc.

If using the terminal, the command should look like this

.. code-block:: bash

	mkdir build_freeRTOS
	cd build_freeRTOS
	cmake -DUA_ARCHITECTURE=freertosLWIP -DUA_ENABLE_AMALGAMATION=ON ../
	make 

Remember, the compilation will fail. That's not a problem, because you need only the generated files (open62541.h and open62541.c) found in the directory where you tried to compile. Import these in your IDE that you're using. 
There is no standard way of doing the following across all IDEs, but you need to do the following configurations in your project:

- Add the open62541.c file for compilation 
- Add the variable UA_ARCHITECTURE_FREERTOSLWIP to the compilation
- Make sure that the open62541.h is in a folder which is included in the compilation.

When compiling LwIP you need a file called lwipopts.h. In this file, you put all the configuration variables. You need to make sure that you have the following configurations there:

.. code-block:: c

	#define LWIP_COMPAT_SOCKETS 0 // Don't do name define-transformation in networking function names.
	#define LWIP_SOCKET 1 // Enable Socket API (normally already set)
	#define LWIP_DNS 1 // enable the lwip_getaddrinfo function, struct addrinfo and more.
	#define SO_REUSE 1 // Allows to set the socket as reusable
	#define LWIP_TIMEVAL_PRIVATE 0 // This is optional. Set this flag if you get a compilation error about redefinition of struct timeval

For freeRTOS there's a similar file called FreeRTOSConfig.h. Usually, you should have an example project with this file. The only two variables that are recommended to check are:

.. code-block:: c

	#define configCHECK_FOR_STACK_OVERFLOW 1
	#define configUSE_MALLOC_FAILED_HOOK 1

Most problems when running the OPC UA server in freeRTOS + LwIP come from the fact that is usually deployed in embedded systems with a limited amount of memory, so these definitions will allow checking if there was a memory problem (will save a lot of effort looking for hidden problems).

Now, you need to add the task that will start the OPC UA server. 

.. code-block:: c

	static void opcua_thread(void *arg){
	
		//The default 64KB of memory for sending and receicing buffer caused problems to many users. With the code below, they are reduced to ~16KB
		UA_UInt32 sendBufferSize = 16000;	//64 KB was too much for my platform
		UA_UInt32 recvBufferSize = 16000;	//64 KB was too much for my platform
		UA_UInt16 portNumber = 4840;
		
		UA_Server* mUaServer = UA_Server_new();
		UA_ServerConfig *uaServerConfig = UA_Server_getConfig(mUaServer);
		UA_ServerConfig_setMinimal(uaServerConfig, portNumber, 0, sendBufferSize, recvBufferSize);
		
		//VERY IMPORTANT: Set the hostname with your IP before starting the server
		UA_ServerConfig_setCustomHostname(uaServerConfig, UA_STRING("192.168.0.102"));
		
		//The rest is the same as the example
		
		UA_Boolean running = true;
		
		// add a variable node to the adresspace
		UA_VariableAttributes attr = UA_VariableAttributes_default;
		UA_Int32 myInteger = 42;
		UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
		attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
		attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
		UA_NodeId myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
		UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
		UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
		UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
		UA_Server_addVariableNode(mUaServer, myIntegerNodeId, parentNodeId,
									parentReferenceNodeId, myIntegerName,
									UA_NODEID_NULL, attr, NULL, NULL);
		
		/* allocations on the heap need to be freed */
		UA_VariableAttributes_clear(&attr);
		UA_NodeId_clear(&myIntegerNodeId);
		UA_QualifiedName_clear(&myIntegerName);
		
		UA_StatusCode retval = UA_Server_run(mUaServer, &running);
		UA_Server_delete(mUaServer);
	}

In your main function, after you initialize the TCP IP stack and all the hardware, you need to add the task:

.. code-block:: c
	
	//8000 is the stack size and 8 is priority. This values might need to be changed according to your project
	if(NULL == sys_thread_new("opcua_thread", opcua_thread, NULL, 8000, 8))
		LWIP_ASSERT("opcua(): Task creation failed.", 0);
		
And lastly, in the same file (or any actually) add:

.. code-block:: c

	void vApplicationMallocFailedHook(){
		for(;;){
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
	
	void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName ){
		for(;;){
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}

And put a breakpoint in each of the vTaskDelay. These functions are called when there's a problem in the heap or the stack. If the program gets here, you have a memory problem.

That's it. Your OPC UA server should run smoothly. If not, as said before, check the discussion in https://github.com/open62541/open62541/pull/2511. If you still have problems, ask there so the discussion remains centralized.
