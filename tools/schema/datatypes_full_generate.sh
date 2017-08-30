#!/bin/bash

cd $( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
egrep "<opc\:(Structured|Enumerated|Opaque)Type" Opc.Ua.Types.bsd | grep -Po 'Name="\K.*?(?=")' | sort -u datatypes_minimal.txt - > datatypes_full.txt
