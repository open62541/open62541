# FileSystem described in Part 20 of OPCUA

## Implementation
This feature is implemented in the driver folder because it has its own lifecycle. It scans the directory for updates (adding or deleting files).

To include this FileSystem. Create a FileServerDriver like you would start a server. This will create a FileServerNode in the node tree. For each Type of Directory you would need one Driver because the driver is the one handling the file operations based on the arch of the directory type.
You can add multiple drivers with the same architecture pointing to the same directories (not recommended).

Currently there is no communication between the drivers if one would like to copy a file from local to cloud or the other way round it is currently not possible.
-> This feature would allow for interesting file management if the server this is running on is capped in size. Then the files could be stored in the cloud and if needed locally.

## Adding new Directory Types
To add new directory types one must add a identifier to the _FileDriverType_ enum in the _ua\_fileserver\_driver.h_.
Then this must get linked to the file system Operations. For the arch type of the directory it is recommended to add a new file to the directoryArch folder where the file operations are implemented. in the common header there is the switch to set the operations based on the type selected.

It is also possible to add directory types like google cloud but those need to be implemented first.