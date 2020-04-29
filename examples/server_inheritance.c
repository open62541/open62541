/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

/**
 * This will create a type structure and some instances of the types:
 *
 * Create a rudimentary objectType
 *
 * Type:
 * + MammalType
 *  v- Class  = "mamalia"
 *  v- Species
 *  o- Abilities
 *      v- MakeSound
 *      v- Breathe = True
 *  + DogType
 *      v- Species = "Canis"
 *      v- Name
 *      o- Abilities
 *          v- MakeSound = "Wuff"
 *           v- FetchNewPaper
 */
static void createMammals(UA_Server *server) {


    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.description = UA_LOCALIZEDTEXT("en-US", "A mammal");
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MammalType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 10000),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "MammalType"), otAttr, NULL, NULL);

    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "This mammals class");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "Class");
    UA_String classVar = UA_STRING("mamalia");
    UA_Variant_setScalar(&vAttr.value, &classVar, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 10001),
                              UA_NODEID_NUMERIC(1, 10000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Class"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "This mammals species");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "Species");
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 10002),
                              UA_NODEID_NUMERIC(1, 10000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Species"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    otAttr = UA_ObjectTypeAttributes_default;
    otAttr.description = UA_LOCALIZEDTEXT("en-US", "A dog, subtype of mammal");
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DogType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 20000),
                                UA_NODEID_NUMERIC(1, 10000),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "DogType"), otAttr, NULL, NULL);

    vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "This dogs species");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "Species");
    UA_String defaultSpecies = UA_STRING("Canis");
    UA_Variant_setScalar(&vAttr.value, &defaultSpecies, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 20001),
                              UA_NODEID_NUMERIC(1, 20000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Species"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "This dogs name");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "Name");
    UA_String defaultName = UA_STRING("unnamed dog");
    UA_Variant_setScalar(&vAttr.value, &defaultName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 20002),
                              UA_NODEID_NUMERIC(1, 20000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Name"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    /* Instatiate a dog named bello:
     * (O) Objects
     *   + O Bello <DogType>
     *     + Age
     *     + Name
     */

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "A dog named Bello");
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Bello");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 0),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Bello"), UA_NODEID_NUMERIC(1, 20000),
                            oAttr, NULL, NULL);

    oAttr = UA_ObjectAttributes_default;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "Another dog");
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Dog2");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 0),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Dog2"), UA_NODEID_NUMERIC(1, 20000),
                            oAttr, NULL, NULL);

    oAttr = UA_ObjectAttributes_default;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "A mmamal");
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Mmamal1");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 0),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "Mammal1"), UA_NODEID_NUMERIC(1, 10000),
                            oAttr, NULL, NULL);

}

/**
 * This method shows the usage of _begin and _finish methods.
 * Normally, if you create an instance of an object type, all its
 * mandatory children are inherited and created.
 * It could be the case that you first need to create a node,
 * add some children with specific IDs and then all the remaining
 * inherited children should be created.
 * For this use-case you can use first the _begin method,
 * which creates the node, including its parent references.
 * Then you can add any children, and then you should
 * call _finish on that node, which then adds all the inherited children.
 *
 * For further details check the example below or the corresponding
 * method documentation.
 *
 * To demonstrate this, we use the following example:
 *
 * + ObjectType
 *      + LampType (Object)
 *          + IsOn (Variable, Boolean, Mandatory)
 *          + Brightness (Variable, UInt16, Mandatory)
 * + Objects
 *      + LampGreen
 *          Should inherit the mandatory IsOn and Brightness with a generated node ID
 *      + LampRed
 *          IsOn should have the node ID 30101, Brightness will be inherited with a generated node ID
 *
 */
static void createCustomInheritance(UA_Server *server) {

    /* Add LampType object type node */

    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.description = UA_LOCALIZEDTEXT("en-US", "A Lamp");
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "LampType");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 30000),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "LampType"), otAttr, NULL, NULL);


    /* Add the two mandatory children, IsOn and Brightness */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "Switch lamp on/off");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "IsOn");
    UA_Boolean isOn = UA_FALSE;
    UA_Variant_setScalar(&vAttr.value, &isOn, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 30001),
                              UA_NODEID_NUMERIC(1, 30000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "IsOn"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(1, 30001),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

    vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "Lamp brightness");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "Brightness");
    UA_UInt16 brightness = 142;
    UA_Variant_setScalar(&vAttr.value, &brightness, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 30002),
                              UA_NODEID_NUMERIC(1, 30000),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "Brightness"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);
    UA_Server_addReference(server, UA_NODEID_NUMERIC(1, 30002),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    
    /* Now we want to inherit all the mandatory children for LampGreen and don't care about the node ids.
     * These will be automatically generated. This will internally call the _begin and _finish methods */

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "A green lamp");
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "LampGreen");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 0),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "LampGreen"), UA_NODEID_NUMERIC(1, 30000),
                            oAttr, NULL, NULL);

    /* For the red lamp we want to set the node ID of the IsOn child manually, thus we need to use
     * the _begin method, add the child and then _finish: */

    /* The call to UA_Server_addNode_begin will create the node and its parent references,
     * but it will not instantiate the mandatory children */
    oAttr = UA_ObjectAttributes_default;
    oAttr.description = UA_LOCALIZEDTEXT("en-US", "A red lamp");
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "LampRed");
    UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT,
                            UA_NODEID_NUMERIC(1, 30100),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "LampRed"),
                            UA_NODEID_NUMERIC(1, 30000),
                            (const UA_NodeAttributes*)&oAttr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                            NULL, NULL);

    /* Now we can add the IsOn with our own node ID */
    vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "Switch lamp on/off");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "IsOn");
    isOn = UA_FALSE;
    UA_Variant_setScalar(&vAttr.value, &isOn, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 30101),
                              UA_NODEID_NUMERIC(1, 30100),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "IsOn"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    /* And then we need to call the UA_Server_addNode_finish which adds all the remaining
     * children and does some further initialization. It will not add the IsNode child,
     * since a child with the same browse name already exists */
    UA_Server_addNode_finish(server, UA_NODEID_NUMERIC(1, 30100));
}

int main(void) {
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    createMammals(server);

    createCustomInheritance(server);

    /* Run the server */
    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
