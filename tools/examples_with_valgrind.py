import os
import subprocess
import time
from collections import defaultdict

example_args = {
        "client_encryption":"opc.tcp://localhost:4840 client_cert.der client_key.der server_cert.der",
        "access_control_client_encrypt":"opc.tcp://localhost:4840 client_cert.der client_key.der server_cert.der",
        "client_event_filter":"opc.tcp://localhost:4840",
        "client_connect":"-username user1 -password password -cert client_cert.der -key client_key.der opc.tcp://localhost:4840",
        "pubsub_nodeset_rt_publisher":"-i lo",
        "pubsub_nodeset_rt_subscriber":"-i lo",
        "pubsub_TSN_loopback":"-i lo",
        "pubsub_TSN_loopback_single_thread":"-i lo",
        "pubsub_TSN_publisher":"-i lo",
        "pubsub_TSN_publisher_multiple_thread":"-i lo",
        "server_encryption":"server_cert.der server_key.der client_cert.der",
        "server_loglevel":"--loglevel=1",
        "ci_server":"4840 server_cert.der server_key.der client_cert.der",
        "server_file_based_config":"server_config.json5"
        }

server_needed_examples = {
        "access_control_client":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "access_control_client_encrypt":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client_async":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client_connect":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client_connect_loop":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client_encryption":"server_encryption server_cert.der server_key.der client_cert.der",
        "client_event_filter":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client_historical":"tutorial_server_historicaldata",
        "client_method_async":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "client_subscription_loop":"ci_server 4840 server_cert.der server_key.der client_cert.der",
        "custom_datatype_client":"custom_datatype_server",
        "discovery_client_find_servers":"discovery_server_lds",
        "discovery_server_register":"discovery_server_lds",
        "pubsub_publish_encrypted":"pubsub_subscribe_encrypted",
        "pubsub_publish_encrypted_sks":"server_pubsub_central_sks server_cert.der server_key.der --enableUnencrypted --enableAnonymous",
        "pubsub_subscribe_encrypted":"pubsub_publish_encrypted",
        "pubsub_subscribe_encrypted_sks":"server_pubsub_central_sks server_cert.der server_key.der --enableUnencrypted --enableAnonymous",
        "pubsub_subscribe_standalone_dataset":"tutorial_pubsub_publish",
        "server_pubsub_publish_rt_level":"server_pubsub_subscribe_rt_level",
        "server_pubsub_rt_information_model":"server_pubsub_subscribe_rt_level",
        "server_pubsub_subscribe_custom_monitoring":"server_pubsub_publish_on_demand",
        "server_pubsub_subscribe_rt_level":"server_pubsub_publish_rt_level",
        "tutorial_client_events":"tutorial_server_events",
        "tutorial_client_firststeps":"tutorial_server_firststeps",
        "tutorial_pubsub_connection":"tutorial_pubsub_subscribe",
        "tutorial_pubsub_mqtt_subscrib":"tutorial_pubsub_mqtt_publish",
        "tutorial_pubsub_publish":"tutorial_pubsub_subscribe",
        "tutorial_pubsub_subscribe":"tutorial_pubsub_publish",
        "tutorial_server_reverseconnect":"ci_server 4841 client_cert.der client_key.der server_cert.der"
}

client_needed_examples = {
        "ci_server":"client",
        "server_ctt":"client",
        "server_inheritance":"client",
        "server_instantiation":"client",
        "server_loglevel":"client",
        "server_mainloop":"client",
        "server_nodeset":"client",
        "server_nodeset_loader":"client",
        "server_nodeset_plcopen":"client",
        "server_nodeset_powerlink":"client",
        "server_settimestamp":"client",
        "server_testnodeset":"client",
        "tutorial_server_monitoreditems":"client",
        "tutorial_server_variabletype":"client",
        "tutorial_server_object":"client",
        "tutorial_server_variable":"client",
        "access_control_server":"access_control_client",
        "custom_datatype_server":"custom_datatype_client",
        "discovery_server_lds":"discovery_client_find_servers",
        "server_encryption":"client_encryption opc.tcp://localhost:4840 client_cert.der client_key.der server_cert.der",
        "server_events_random":"tutorial_client_events opc.tcp://localhost:4840",
        "server_pubsub_central_sks":"pubsub_publish_encrypted_sks",
        "tutorial_server_alarms_conditions":"tutorial_client_events opc.tcp://localhost:4840",
        "tutorial_server_datasource":"tutorial_client_events opc.tcp://localhost:4840",
        "tutorial_server_firststeps":"tutorial_client_firststeps",
        "tutorial_server_historicaldata":"client_historical",
        "tutorial_server_historicaldata_circular":"client_historical",
        "tutorial_server_method":"client_method_async",
        "tutorial_server_method_async":"client_method_async"
}

blacklist = {
        "pubsub_TSN_loopback":1,
        "pubsub_TSN_loopback_single_thread":1,
        "pubsub_TSN_publisher":1,
        "pubsub_TSN_publisher_multiple_thread":1,
        "pubsub_nodeset_rt_publisher":1,
        "pubsub_nodeset_rt_subscriber":1,
        "access_control_client_encrypt":1,
        "client_encryption":1,
        "client_historical":1,
        "client_subscription_loop":1,
        "client_method_async":1,
        "pubsub_subscribe_encrypted":1
}

# Run each example with valgrind.
# Wait 10 seconds and send the SIGINT signal.
# Wait for the process to terminate and collect the exit status.
# Abort when the exit status is non-null.

os.chdir("../build")
current_dir = os.getcwd()
print(f"Current directory: {current_dir}")
example_dir = os.path.join(current_dir, "bin", "examples")
examples = os.listdir(example_dir)

# skipping examples that are in the blacklist
for example in examples:
    if example in blacklist:
        print(f"Skipping {example}")
        continue

    # get the arguments for the example
    args = example_args.get(example)
    cmd = ["valgrind", "--errors-for-leak-kinds=all", "--leak-check=full", "--error-exitcode=1337", "./bin/examples/"+example]
    if args:
        args_list = args.split()
        cmd += args_list
    print(f"Args for {example}: {cmd} ")

    # check if the example needs a server or client to be running
    server_needed = server_needed_examples.get(example)
    client_needed = client_needed_examples.get(example)
    
    #start the server if needed
    server_process = None
    if server_needed:
        server_cmd = server_needed.split()
        server_cmd[0] = "./bin/examples/" + server_cmd[0]
        print(f"Starting server {server_cmd} from {os.getcwd()}")
        server_process = subprocess.Popen(server_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(10)
    
    # start the example
    process = subprocess.Popen(cmd)
    time.sleep(10)
    
    # start the client if needed
    client_process = None
    if client_needed:
        client_cmd = client_needed.split()
        client_cmd[0] = "./bin/examples/" + client_cmd[0]
        print(f"Starting client {client_cmd}")
        client_process = subprocess.Popen(client_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    # wait 20 seconds and send the SIGINT signal
    time.sleep(20)
    print(f"Sending SIGINT to {process.pid}")
    process.send_signal(subprocess.signal.SIGINT)
    
    tries = 0
    while process.poll() is None:
        time.sleep(20)
        process.send_signal(subprocess.signal.SIGINT)
        if tries > 5:
            print(f"Process {process.pid} is still running after 5 tries. Terminating process.")
            process.kill()
        tries += 1

    # save the exit code
    exit_code = process.wait()
    if exit_code == 1337:
        print(f"Processing {example} failed with valgrind issues")
        exit(exit_code)
    if exit_code != 0:
        print(f"The application returned exit code {exit_code} but valgrind has no issue")

    # terminate the server and client
    if server_process:
        print(f"Sending SIGKILL to server {server_process.pid}")
        server_process.kill()
        server_process.wait()
    
    if client_process:
        print(f"Sending SIGKILL to client {client_process.pid}")
        client_process.kill()
        client_process.wait()
