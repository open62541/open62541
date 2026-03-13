#!/usr/bin/env python3
"""
Validate open62541 JSON/JSON5 configuration files against JSON schemas.
"""

import json
import json5
import sys
from pathlib import Path
from jsonschema import Draft7Validator

def load_json5(filepath):
    """Load a JSON5 file and convert to Python dict."""
    with open(filepath, 'r') as f:
        try:
            return json5.load(f)
        except Exception as e:
            print(f"  ❌ JSON5 parsing error: {e}")
            return None

def validate_config(config_file, schema_file, base_dir):
    """Validate a configuration file against a schema."""
    try:
        rel_path = config_file.relative_to(base_dir)
    except ValueError:
        rel_path = config_file.name
    print(f"\nValidating: {rel_path}")
    print(f"   Schema: {schema_file.name}")

    # Load the schema
    with open(schema_file, 'r') as f:
        schema = json.load(f)

    # Load the config
    config = load_json5(config_file)
    if config is None:
        return False

    # Validate
    validator = Draft7Validator(schema)
    errors = list(validator.iter_errors(config))

    if errors:
        print(f"  ❌ Validation failed with {len(errors)} error(s):")
        for i, error in enumerate(errors, 1):
            path = " -> ".join(str(p) for p in error.path) if error.path else "root"
            print(f"     {i}. At '{path}': {error.message}")
        return False
    else:
        print("  ✅ Valid!")
        return True

def find_config_files(base_dir):
    """Find all JSON5 configuration files in tests and examples directories."""
    config_files = {
        'client': [],
        'server': []
    }

    # Search in tests and examples directories
    search_dirs = [base_dir / "tests", base_dir / "examples"]

    for search_dir in search_dirs:
        if not search_dir.exists():
            continue

        # Find all .json5 files recursively
        for json5_file in search_dir.rglob("*.json5"):
            # Determine type based on path
            path_str = str(json5_file)
            if "client" in path_str.lower():
                config_files['client'].append(json5_file)
            elif "server" in path_str.lower():
                config_files['server'].append(json5_file)

    # Sort files for consistent output
    config_files['client'].sort()
    config_files['server'].sort()

    return config_files

def main():
    base_dir = Path(__file__).parent.parent
    tools_dir = base_dir / "tools"

    # Schema files
    client_schema = tools_dir / "client_config_schema.json"
    server_schema = tools_dir / "server_config_schema.json"

    # Auto-discover configuration files
    config_files = find_config_files(base_dir)

    print("=" * 70)
    print("Validating open62541 Configuration Files")
    print("=" * 70)
    print(f"\nFound {len(config_files['client'])} client config(s) and {len(config_files['server'])} server config(s)")

    all_valid = True

    # Validate client configs
    if config_files['client']:
        print("\n" + "=" * 70)
        print("CLIENT CONFIGURATIONS")
        print("=" * 70)
        for config_file in config_files['client']:
            if not validate_config(config_file, client_schema, base_dir):
                all_valid = False
    else:
        print("\n⚠️  No client configuration files found")

    # Validate server configs
    if config_files['server']:
        print("\n" + "=" * 70)
        print("SERVER CONFIGURATIONS")
        print("=" * 70)
        for config_file in config_files['server']:
            if not validate_config(config_file, server_schema, base_dir):
                all_valid = False
    else:
        print("\n⚠️  No server configuration files found")

    print("\n" + "=" * 70)
    if all_valid:
        print("✅ All configuration files are valid!")
        print("=" * 70)
        return 0
    else:
        print("❌ Some configuration files have validation errors")
        print("=" * 70)
        return 1

if __name__ == "__main__":
    sys.exit(main())
