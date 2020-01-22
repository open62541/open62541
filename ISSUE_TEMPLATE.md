<!--
!ATTENTION!
Please read the following page carefully and provide us with all the
information requested:
https://github.com/open62541/open62541/wiki/Writing-Good-Issue-Reports

Use Github Markdown to format your text:
https://help.github.com/articles/basic-writing-and-formatting-syntax/

Fill out the sections and checklist below (add text at the end of each line).

!ATTENTION!
--------------------------------------------------------------------------------
-->

## Description

## Background Information / Reproduction Steps



Used CMake options:

<!-- 

Include all CMake options here, which you modified or used for your build.

If you are using cmake-gui, go to "Tools > Show my Changes" and paste the content of "Command Line Options"

On the command line use `cmake -L` (or `cmake -LA` if you changed advanced variables)
-->

```bash
cmake -DUA_NAMESPACE_ZERO=<YOUR_OPTION> <ANY_OTHER_OPTIONS> ..
```

## Checklist

Please provide the following information:

 - [ ] open62541 Version (release number or git tag):
 - [ ] Other OPC UA SDKs used (client or server): 
 - [ ] Operating system:
 - [ ] Logs (with `UA_LOGLEVEL` set as low as necessary) attached
 - [ ] Wireshark network dump attached
 - [ ] Self-contained code example attached
 - [ ] Critical issue
