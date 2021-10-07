@ECHO off
REM ****************************************************************************************************************
REM ** --
REM ** This script demonstrates how to use the model compiler to generate source code from a variety
REM ** of XML files that adhere to the 'Nodeset2.xml' format. Please refer to the UA Specifications Part 6
REM ** for more information.
REM ** --
REM ****************************************************************************************************************
SETLOCAL

set SRCDIR=%~dp0
set INPUT=%SRCDIR%
set OUTPUT=X:\Work\SysAdmin\git\Web_Website\UA\schemas

IF NOT "%1"=="" (set OUTPUT=%OUTPUT%\%1) else (set OUTPUT=%OUTPUT%\1.04)

IF NOT "%2"=="" (
	IF NOT EXIST "%OUTPUT%\%2" MKDIR "%OUTPUT%\%2" 
	MOVE /Y "%OUTPUT%\*.*" "%OUTPUT%\%2" 
)

ECHO Copying CSV files from %INPUT% to %OUTPUT%
ECHO ON

DEL /Q "%OUTPUT%\*.*"

COPY "%INPUT%\Schema\NodeIds.csv" "%OUTPUT%\NodeIds.csv"
COPY "%INPUT%\Schema\AttributeIds.csv" "%OUTPUT%\AttributeIds.csv"
COPY "%INPUT%\Schema\StatusCode.csv" "%OUTPUT%\StatusCode.csv"
COPY "%INPUT%\Schema\UANodeSet.xsd" "%OUTPUT%\UANodeSet.xsd"
COPY "%INPUT%\Schema\SecuredApplication.xsd" "%OUTPUT%\SecuredApplication.xsd"
COPY "%INPUT%\Schema\OPCBinarySchema.xsd" "%OUTPUT%\OPCBinarySchema.xsd"
COPY "%INPUT%\Schema\ServerCapabilities.csv" "%OUTPUT%\ServerCapabilities.csv"
COPY "%INPUT%\Schema\Opc.Ua.NodeSet2.xml" "%OUTPUT%\Opc.Ua.NodeSet2.xml"
COPY "%INPUT%\Schema\Opc.Ua.NodeSet2.Services.xml" "%OUTPUT%\Opc.Ua.NodeSet2.Services.xml"
COPY "%INPUT%\Schema\Opc.Ua.Types.bsd" "%OUTPUT%\Opc.Ua.Types.bsd"
COPY "%INPUT%\Schema\Opc.Ua.Types.xsd" "%OUTPUT%\Opc.Ua.Types.xsd"
COPY "%INPUT%\Schema\rec20_latest_a1.csv" "%OUTPUT%\rec20_latest_a1.csv
COPY "%INPUT%\Schema\rec20_latest_a2-3.csv" "%OUTPUT%\rec20_latest_a2-3.csv"
COPY "%INPUT%\Schema\UNECE_to_OPCUA.csv" "%OUTPUT%\UNECE_to_OPCUA.csv"

COPY "%INPUT%\DotNet\Opc.Ua.Endpoints.wsdl" "%OUTPUT%\Opc.Ua.Endpoints.wsdl"
COPY "%INPUT%\DotNet\Opc.Ua.Services.wsdl" "%OUTPUT%\Opc.Ua.Services.wsdl"

COPY "%INPUT%\GDS\Opc.Ua.Gds.Types.bsd" "%OUTPUT%\Opc.Ua.Gds.Types.bsd"
COPY "%INPUT%\GDS\Opc.Ua.Gds.Types.xsd" "%OUTPUT%\Opc.Ua.Gds.Types.xsd"
COPY "%INPUT%\GDS\Opc.Ua.Gds.NodeSet2.xml" "%OUTPUT%\Opc.Ua.Gds.NodeSet2.xml"
COPY "%INPUT%\GDS\OpcUaGdsModel.csv" "%OUTPUT%\Opc.Ua.Gds.NodeIds.csv"
	
@ECHO OFF

:theEnd
ENDLOCAL