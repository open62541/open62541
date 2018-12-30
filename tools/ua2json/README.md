# ua2json

ua2json is a command line tool to translate between OPC UA JSON encoding and OPC
UA binary encoding. ua2json follows the tradition of shell-based unix-tools and
provides translation between the binary and JSON encoding formats of OPC UA
messages as a reusable building block. Input can be piped through ua2json from
stdin to stdout. File input and output is possible as well.

At the core of the OPC UA protocol lies a type system in which the protocol
messages are defined. The built-in data types include integers, strings, and so
on. From these, more complex structures are assembled. For example the
`ReadRequest` and `ReadResponse` message pair.

## Usage

```
Usage: ua2json [encode|decode] [-t dataType] [-o outputFile] [inputFile]
- encode/decode: Translate UA binary input to UA JSON / Translate UA JSON input to UA binary (required)
- dataType: UA DataType of the input (default: Variant)
- outputFile: Output target (default: write to stdout)
- inputFile: Input source (default: read from stdin)
```

## Examples

Take the following JSON encoding of a Variant data type instance with a 2x4
matrix of numerical values. Variants can encapsulate scalars and
multi-dimensional arrays of any data type.

```json
{
    "Type": 3,
    "Body": [1,2,3,4,5,6,7,8],
    "Dimension": [2, 4]
}
```

Piping this JSON-encoding through ua2json (and the xxd tool to print the output
as hex) yields the binary OPC UA encoding.

```bash
$ cat variant.json | ua2json decode -t Variant | xxd
00000000: c308 0000 0001 0203 0405 0607 0802 0000  ................
00000010: 0002 0000 0004 0000 00                   .........
```

The inverse transformation returns the original JSON (modulo pretty-printing).

```bash
$ cat variant.bin | ua2json encode -t Variant
```