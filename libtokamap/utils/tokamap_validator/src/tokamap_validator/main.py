from pathlib import Path
import argparse
from .validate import Validator
import json


def validate_mapping(mapping_dir, mappings_validator, globals_validator, verbose):
    globals_file = mapping_dir / 'globals.json'
    globals_validator.validate(globals_file)

    mappings_file = mapping_dir / 'mappings.json'
    mappings_validator.validate(mappings_file)


def run():
    parser = argparse.ArgumentParser(description='Validate a TokaMap mapping directory')
    parser.add_argument('directory', help='TokaMap mapping directory to validate')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    args = parser.parse_args()

    mappings_validator = Validator('../../schemas/mappings.schema.json')
    globals_validator = Validator('../../schemas/globals.schema.json')

    root = Path(args.directory)
    if not root.is_dir():
        print(f"Argument '{args.directory}' is not a directory.")
        return

    config_file = root / 'mappings.cfg.json'
    if not config_file.exists():
        print(f"Configuration file '{config_file}' does not exist.")
        return

    if args.verbose:
        print("Reading configuration file 'mappings.cfg.json'")

    with open(config_file, 'r') as f:
        config = json.load(f)

    # TODO: Update once we've figured out how the config file is structured
    mappings = config["mappings"]["3.39.0"]

    if args.verbose:
        print(f"Mappings found: {mappings}")

    toplevel_globals = root / 'globals.json'
    if not toplevel_globals.exists():
        print(f"Root level globals file '{toplevel_globals}' does not exist.")
        return

    if args.verbose:
        print("Validating top-level globals ... ", end="")
    globals_validator.validate(toplevel_globals)
    if args.verbose:
        print("ok")

    for mapping in config["mappings"]["3.39.0"]:
        if args.verbose:
            print(f"Validating mapping '{mapping}' ... ", end="")
        validate_mapping(root /mapping, mappings_validator, globals_validator, args.verbose)
        if args.verbose:
            print("ok")
