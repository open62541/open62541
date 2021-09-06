# open62541 Third-Party libraries

Specific optional features are dependent on third-party libraries. 
Any third-party library which may be used is inside this `/deps` folder.

Up to now all these libraries have a less strict License compared to MPL 2.0.
Anyways you should make sure that the corresponding third-party license matches your needs.

Here's a list of third party libraries:

| Library         | License          | Description                                   |
|-----------------|------------------|-----------------------------------------------|
| jsmn            | MIT              | json parser                                   |
| mdnsd           | BSD-3-Clause     | mDNS library                                  |
| ua-nodeset      | MIT              | Official OPC UA Nodeset files by the OPCF     |
| atoi            | MIT              | Char to int conversion, from musl             |
| base64          | BSD              | Base64 encoding and decoding                  |
| itoa            | MIT              | Int to char conversion                        |
| ms_stdint       | BSD-3-Clause     | Replacement for stdint on older Visual Studio |
| open62541_queue | BSD-3-Clause     | FIFO and LIFO queue implementation            |
| pcg_basic       | Apache License 2 | Random Number Generation                      |
| string_escape   | MIT              | utf8 encoding and decoding                    |
| ziptree         | MPL 2.0          | Reusable zip tree implementation              |
