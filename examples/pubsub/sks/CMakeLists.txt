########################################
# PubSub Security Key Service Examples #
########################################
if(UA_NAMESPACE_ZERO STREQUAL "FULL")
    add_example(server_pubsub_central_sks server_pubsub_central_sks.c)  
endif()
add_example(pubsub_publish_encrypted_sks pubsub_publish_encrypted_sks.c)
add_example(pubsub_subscribe_encrypted_sks pubsub_subscribe_encrypted_sks.c)
