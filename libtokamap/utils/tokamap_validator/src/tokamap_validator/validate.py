import json
import jsonschema

class Validator:
    def __init__(self, schema_file):
        with open(schema_file, 'r') as f:
            self.schema = json.load(f)

    def validate(self, mappings_file):
        with open(mappings_file, 'r') as f:
            mappings = json.load(f)
            jsonschema.validate(mappings, self.schema)
