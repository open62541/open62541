# open62541 Architectures 

The `arch` folder contains all the architecture-specific code for a given operating system.

The list of supported architectures is also available as a CMake option.

## Adding new architectures

To port to a new architecture you should follow these steps:

1. Create a folder with your architecture, let's call it new_arch

2. In the CMakeLists.txt file located next to this file, add `add_subdirectory(new_arch)` at the end of it

3. Create a CMakeLists.txt file in the new_arch folder

4. Use the following template for it (remember that when you see new_arch you should replace with the name of your architecture)
    ```C
    # ---------------------------------------------------
    # ---- Beginning of the CMakeLists.txt template -----
    # ---------------------------------------------------
    
    SET(SOURCE_GROUP ${SOURCE_GROUP}\\new_arch)
    
    ua_add_architecture("new_arch")
    
    if("${UA_ARCHITECTURE}" STREQUAL "new_arch")
    
        ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
        ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
        
        #
        # Add here below all the things that are specific for your architecture
        #
        
        #You can use the following available CMake functions:
         
        #ua_include_directories() include some directories specific to your architecture when compiling the open62541 stack
        #ua_architecture_remove_definitions() remove compiler flags from the general ../../CMakeLists.txt file that won't work with your architecture
        #ua_architecture_add_definitions() add compiler flags that your architecture needs
        #ua_architecture_append_to_library() add libraries to be linked to the open62541 that are needed by your architecture
        #ua_add_architecture_header() add header files to compilation (Don't add the file ua_architecture.h)
        #ua_add_architecture_file() add .c files to compilation    
        
    endif()
    
    # ---------------------------------------------------
    # ---- End of the CMakeLists.txt template -----
    # ---------------------------------------------------
    ```
5. Create a ua_clock.c file that implements the following functions defined in open62541/types.h:

   * UA_DateTime UA_DateTime_now(void);
   
   * UA_Int64 UA_DateTime_localTimeUtcOffset(void);
   
   * UA_DateTime UA_DateTime_nowMonotonic(void);

6. Create a file in the folder new_arch called ua_architecture.h

7. Use the following template for it:  

   * Change YEAR, YOUR_NAME and YOUR_COMPANY in the header
   
   * Change NEW_ARCH at the beginning in PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_ for your own name in uppercase  
   
    ```C
    /* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
     * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
     *
     *    Copyright YEAR (c) YOUR_NAME, YOUR_COMPANY
     */
    
    #ifndef PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_
    #define PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_
    
    /*
    * Define and include all that's needed for your architecture
    */
    
    /*
    * Define OPTVAL_TYPE for non windows systems. In doubt, use int //TODO: Is this really necessary
    */
    
    /*
    * Define the following network options
    */
    
    
    //#define UA_IPV6 1 //or 0
    //#define UA_SOCKET
    //#define UA_INVALID_SOCKET
    //#define UA_ERRNO
    //#define UA_INTERRUPTED
    //#define UA_AGAIN
    //#define UA_WOULDBLOCK
    //#define UA_INTERRUPTED
    
    /*
    * Define the ua_getnameinfo if your architecture supports it
    */
    
    /*
    * Use #define for the functions defined in ua_architecture_functions.h
    * or implement them in a ua_architecture_functions.c file and 
    * put it in your new_arch folder and add it in the CMakeLists.txt file 
    * using ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
    */ 
    
    /*
    * Define UA_LOG_SOCKET_ERRNO_WRAP(LOG) which prints the string error given a char* errno_str variable
    * Do the same for UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) for errors related to getaddrinfo
    */
    
    #include <open62541/architecture_functions.h>
    
    #endif /* PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_ */
    
    ```
