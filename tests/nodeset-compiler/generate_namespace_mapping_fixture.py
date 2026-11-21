#!/usr/bin/env python3
import sys
from xml.sax.saxutils import escape
from pathlib import Path
payload = "Grüße<&\\" * 4000
Path(sys.argv[1]).write_text('<?xml version="1.0" encoding="utf-8"?>\n<UANodeSet xmlns="http://opcfoundation.org/UA/2011/03/UANodeSet.xsd" xmlns:uax="http://opcfoundation.org/UA/2008/02/Types.xsd">\n'+ '<NamespaceUris><Uri>urn:open62541:test:map-a</Uri></NamespaceUris>' +
    '<UAVariable NodeId="ns=1;i=1005" BrowseName="1:LargeText" DataType="i=12" ValueRank="-1">'
    '<DisplayName>LargeText</DisplayName><References><Reference ReferenceType="i=40">i=63</Reference>'
    '<Reference ReferenceType="i=35" IsForward="false">i=85</Reference></References>'
    '<Value><uax:String>' + escape(payload) + '</uax:String></Value></UAVariable></UANodeSet>', encoding='utf-8')
