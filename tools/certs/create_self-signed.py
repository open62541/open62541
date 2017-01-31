import sys
import os
import shutil
import socket

if len(sys.argv) < 2:
    sys.exit('Usage: %s directory to output certificates' % sys.argv[0])

if not os.path.exists(sys.argv[1]):
    sys.exit('ERROR: Directory %s was not found!' % sys.argv[1])
    
os.chdir(os.path.dirname(os.path.abspath(__file__)))

os.environ['HOSTNAME'] = socket.gethostname()
os.environ['OPENSSL_CONF'] = os.path.join(os.getcwd(), "localhost.cnf")

os.system("""openssl genrsa -out ca.key 2048""")
os.system("""openssl req \
	-x509 \
	-new \
	-nodes \
	-key ca.key \
	-days 3650 \
	-subj "/C=DE/O=open62541/CN=open62541.org" \
	-out ca.crt""")
os.system("""openssl req \
    -new \
    -newkey rsa:2048 \
    -nodes \
    -subj "/C=DE/O=open62541/CN=open62541Server@localhost" \
    -config localhost.cnf \
    -keyout localhost.key \
    -out localhost.csr""")
os.system("""openssl x509 -req \
	-days 3650 \
	-in localhost.csr \
	-CA ca.crt \
	-CAkey ca.key \
	-CAcreateserial \
	-out localhost.crt \
	-extensions v3_ca \
	-extfile localhost.cnf""")
os.system("""openssl x509 -in localhost.crt -outform der -out server_cert.der""")
#we will need these files later
os.remove("localhost.key") #we will need it later
os.remove("localhost.crt")
os.remove("localhost.csr")
os.remove("ca.key")
os.remove("ca.srl")

if os.path.isfile(os.path.join(sys.argv[1], "server_cert.der")):
	os.remove(os.path.join(sys.argv[1], "server_cert.der"))
shutil.move("server_cert.der", sys.argv[1])
if os.path.isfile(os.path.join(sys.argv[1], "ca.crt")):
	os.remove(os.path.join(sys.argv[1], "ca.crt"))
shutil.move("ca.crt", sys.argv[1])

print("Certificates generated in " + sys.argv[1])
