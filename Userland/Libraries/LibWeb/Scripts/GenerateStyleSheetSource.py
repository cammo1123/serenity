#!/usr/bin/env python3

import argparse

parser = argparse.ArgumentParser(description='Generate a C++ string from a CSS file')
parser.add_argument('variable_name', metavar='var_name', type=str,
                    help='the name of the C++ string variable to generate')
parser.add_argument('input_file', metavar='input_file', type=str,
                    help='the path to the CSS file to read from')
parser.add_argument('output_file', metavar='output_file', type=str,
                    help='the path to the C++ file to write to')

args = parser.parse_args()

with open(args.input_file, 'r') as f:
    css_lines = f.readlines()

css_lines = [line.rstrip() for line in css_lines if not line.startswith('#')]

cpp_string = '\\\n'.join(css_lines)

with open(args.output_file, 'w') as f:
    f.write('#include <AK/StringView.h>\n')
    f.write('namespace Web::CSS {\n')
    f.write(f'extern StringView {args.variable_name};\n')
    f.write(f'StringView {args.variable_name} = "{cpp_string}"sv;\n')
    f.write('}\n')
