#!/bin/sh

if [ -z "$1" ]
then 
	echo "Generates a self-signed DER certificate for localhost\n"
	echo "usage ./create_self-signed.sh <output directory>"
	exit 1
fi

#cd in the directory where script is located
cd "$(dirname "$0")"

openssl req \
    -new \
    -newkey rsa:1024 \
    -nodes \
    -subj "/C=DE/ST=/L=/O=open62541/CN=www.open62541.org" \
    -config localhost.cnf \
    -keyout localhost.key \
    -out localhost.csr
openssl x509 -req \
	-days 3650 \
	-in localhost.csr \
	-signkey localhost.key \
	-out localhost.crt \
	-extensions v3_req \
	-extfile localhost.cnf
openssl x509 -in localhost.crt -outform der -out localhost.der
rm localhost.key #we will need it later
rm localhost.crt
rm localhost.csr
if [ -n "$1" ]
then
	mv localhost.der $1
fi
