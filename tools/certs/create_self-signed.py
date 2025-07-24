#!/usr/bin/env python3
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2019 (c) Kalycito Infotech Private Limited
# Modified 2025 (c) Construction Future Lab

"""
Open62541 Certificate Generation Script
---------------------------------------
Generates an X.509 certificate and private key (DER format) using OpenSSL.

Requirements:
- Python 3.5+
- OpenSSL command line tool
- netifaces package (pip install netifaces)

Features:
- Robust error handling with subprocess
- Automatic IP address detection
- Configurable certificate parameters
- Cross-platform compatibility
- Fail-fast execution with proper cleanup
"""

import sys
import os
import socket
import argparse
import subprocess
import shutil
from typing import List, Optional

# Check Python version compatibility
if sys.version_info < (3, 5):
    sys.exit("ERROR: This script requires Python 3.5 or higher")

try:
    import netifaces
except ImportError:
    sys.exit("ERROR: Missing required dependency 'netifaces'. Install with: pip install netifaces")


def check_dependencies():
    """Check if required external tools are available."""
    if not shutil.which("openssl"):
        raise RuntimeError("OpenSSL not found in PATH. Please install OpenSSL.")


def run_command(cmd: List[str], description: str = "Command") -> None:
    """
    Execute a subprocess command with comprehensive logging and error handling.
    
    Args:
        cmd: List of command arguments
        description: Human-readable description of the command
        
    Raises:
        RuntimeError: If command fails with non-zero exit code
    """
    print(f"[RUN] {description}")
    print(f"$ {' '.join(cmd)}")
    
    try:
        result = subprocess.run(
            cmd, 
            capture_output=True, 
            text=True, 
            check=False,
            timeout=30  # Prevent hanging
        )
    except subprocess.TimeoutExpired:
        raise RuntimeError(f"[ERROR] {description} timed out after 30 seconds")
    except FileNotFoundError:
        raise RuntimeError(f"[ERROR] Command not found: {cmd[0]}")

    # Log output
    if result.stdout.strip():
        print(f"stdout: {result.stdout.strip()}")
    if result.stderr.strip():
        print(f"stderr: {result.stderr.strip()}")

    if result.returncode != 0:
        raise RuntimeError(f"[ERROR] {description} failed with exit code {result.returncode}")

    print(f"[OK] {description} completed successfully\n")


def safe_remove_file(filepath: str) -> None:
    """
    Safely remove a file, ignoring FileNotFoundError.
    
    Args:
        filepath: Path to file to remove
    """
    try:
        os.remove(filepath)
        print(f"[CLEAN] Removed: {filepath}")
    except FileNotFoundError:
        print(f"[SKIP] File not found: {filepath}")
    except PermissionError:
        print(f"[WARN] Permission denied removing: {filepath}")


def get_network_ips(max_ips: int = 2) -> List[str]:
    """
    Auto-detect IPv4 addresses from available network interfaces.
    
    Args:
        max_ips: Maximum number of IP addresses to return
        
    Returns:
        List of IP addresses (padded with 127.0.0.1 if needed)
    """
    ips = []
    
    try:
        for interface in netifaces.interfaces():
            # Skip loopback interface
            if interface == "lo":
                continue
                
            # Check if interface has IPv4 address
            if_addrs = netifaces.ifaddresses(interface)
            if netifaces.AF_INET in if_addrs:
                ip = if_addrs[netifaces.AF_INET][0]['addr']
                ips.append(ip)
                
                if len(ips) >= max_ips:
                    break
    except Exception as e:
        print(f"[WARN] Error detecting network interfaces: {e}")
    
    # Pad with localhost if we don't have enough IPs
    while len(ips) < max_ips:
        ips.append("127.0.0.1")
    
    return ips[:max_ips]


def validate_parameters(keysize: int, outdir: str) -> None:
    """
    Validate input parameters and provide warnings.
    
    Args:
        keysize: RSA key size in bits
        outdir: Output directory path
        
    Raises:
        ValueError: If parameters are invalid
    """
    if not os.path.exists(outdir):
        raise ValueError(f"Output directory does not exist: {outdir}")
    
    if not os.access(outdir, os.W_OK):
        raise ValueError(f"Output directory is not writable: {outdir}")
    
    if keysize < 1024:
        print("[WARN] Key size less than 1024 bits is not secure")
    elif keysize > 4096:
        print("[WARN] Key size greater than 4096 bits may be slow")
    
    if keysize % 8 != 0:
        raise ValueError("Key size must be a multiple of 8")


def setup_environment(uri: str, ips: List[str]) -> None:
    """
    Set up environment variables for OpenSSL configuration.
    
    Args:
        uri: Application URI
        ips: List of IP addresses
    """
    os.environ['URI1'] = uri
    os.environ['HOSTNAME'] = socket.gethostname()
    os.environ['IPADDRESS1'] = ips[0] if ips else "127.0.0.1"
    os.environ['IPADDRESS2'] = ips[1] if len(ips) > 1 else "127.0.0.1"
    
    print(f"[INFO] Environment configured:")
    print(f"  URI: {uri}")
    print(f"  Hostname: {os.environ['HOSTNAME']}")
    print(f"  IP1: {os.environ['IPADDRESS1']}")
    print(f"  IP2: {os.environ['IPADDRESS2']}")


def generate_certificates(openssl_conf: str, keysize: int, certificatename: str) -> None:
    """
    Generate X.509 certificates using OpenSSL.
    
    Args:
        openssl_conf: Path to OpenSSL configuration file
        keysize: RSA key size in bits
        certificatename: Base name for certificate files
    """
    try:
        # Step 1: Generate PEM format certificate and key
        run_command([
            "openssl", "req",
            "-config", openssl_conf,
            "-new", "-nodes", "-x509", "-sha256",
            "-newkey", f"rsa:{keysize}",
            "-keyout", "localhost.key",
            "-days", "365",
            "-subj", "/C=DE/L=Here/O=open62541/CN=open62541Server@localhost",
            "-out", "localhost.crt"
        ], "Generate certificate and private key")

        # Step 2: Convert certificate to DER format
        run_command([
            "openssl", "x509",
            "-in", "localhost.crt",
            "-outform", "der",
            "-out", f"{certificatename}_cert.der"
        ], "Convert certificate to DER format")

        # Step 3: Convert private key to DER format
        run_command([
            "openssl", "rsa",
            "-inform", "PEM",
            "-in", "localhost.key",
            "-outform", "DER",
            "-out", f"{certificatename}_key.der"
        ], "Convert private key to DER format")

        # Step 4: Clean up intermediate files
        for temp_file in ["localhost.key", "localhost.crt"]:
            safe_remove_file(temp_file)

    except Exception as e:
        # Clean up all files on failure
        print(f"\n[FAIL] Certificate generation failed: {e}")
        cleanup_files = [
            "localhost.key", "localhost.crt",
            f"{certificatename}_cert.der", f"{certificatename}_key.der"
        ]
        for cleanup_file in cleanup_files:
            safe_remove_file(cleanup_file)
        raise


def main():
    """Main function to orchestrate certificate generation."""
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Generate self-signed X.509 certificates for Open62541",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          # Generate in current directory
  %(prog)s /path/to/certs           # Generate in specific directory
  %(prog)s -k 4096 -c mycert        # Custom key size and certificate name
  %(prog)s -u "urn:myapp:server"    # Custom application URI
        """
    )
    
    parser.add_argument(
        'outdir', 
        type=str, 
        nargs='?', 
        default=os.getcwd(),
        metavar='<OutputDirectory>',
        help='Output directory for certificates (default: current directory)'
    )
    
    parser.add_argument(
        '-u', '--uri',
        metavar="<ApplicationUri>",
        type=str,
        default="urn:open62541.unconfigured.application",
        dest="uri",
        help='Application URI for certificate (default: urn:open62541.unconfigured.application)'
    )
    
    parser.add_argument(
        '-k', '--keysize',
        metavar="<KeySize>",
        type=int,
        default=2048,
        dest="keysize",
        help='RSA key size in bits (default: 2048)'
    )
    
    parser.add_argument(
        '-c', '--certificatename',
        metavar="<CertificateName>",
        type=str,
        default="server",
        dest="certificatename",
        help='Base name for certificate files (default: server)'
    )
    
    args = parser.parse_args()

    try:
        # Check dependencies
        check_dependencies()
        
        # Validate parameters
        validate_parameters(args.keysize, args.outdir)
        
        # Find OpenSSL configuration file
        script_dir = os.path.dirname(os.path.abspath(__file__))
        openssl_conf = os.path.join(script_dir, "localhost.cnf")
        
        if not os.path.isfile(openssl_conf):
            raise FileNotFoundError(f"OpenSSL config file not found: {openssl_conf}")
        
        # Auto-detect network configuration
        network_ips = get_network_ips(2)
        setup_environment(args.uri, network_ips)
        
        # Change to output directory
        original_dir = os.getcwd()
        os.chdir(os.path.abspath(args.outdir))
        
        try:
            # Generate certificates
            generate_certificates(openssl_conf, args.keysize, args.certificatename)
            
            # Success summary
            print(f"\n[SUCCESS] Certificates generated successfully in: {args.outdir}")
            print(f"  Certificate: {args.certificatename}_cert.der")
            print(f"  Private Key: {args.certificatename}_key.der")
            print(f"  Key Size: {args.keysize} bits")
            print(f"  Valid for: 365 days")
            
        finally:
            # Always restore original directory
            os.chdir(original_dir)
            
    except KeyboardInterrupt:
        print("\n[ABORT] Operation cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n[ERROR] {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
