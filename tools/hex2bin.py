# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this 
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

import sys
import os
from io import open
import binascii
import re

def clean_line(string):
    comment_re = re.compile("/\*.*?\*/") # matches C-style comments /* */ at the end of a line
    return re.sub(comment_re, "" ,string).replace(" ","").replace("\n","")

if len(sys.argv) < 2:
    print("Usage: python hex2bin.py file1.hex file2.hex ...")
    exit(0)

filenames = sys.argv[1:]
for f in filenames:
    bn = os.path.basename(f)
    with open(f, mode="rt") as ff:
        with open(bn[:-4] + ".bin", "wb") as out:
            lines = ff.readlines()
            for l in lines:
                c = clean_line(l)
                out.write(binascii.unhexlify(c))
        
