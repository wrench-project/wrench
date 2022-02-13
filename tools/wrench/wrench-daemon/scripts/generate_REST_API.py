#!/usr/bin/env python3
import sys
import re
import json


def generate_documentation_item(spec):
    documentation_string = "<hr>"
    documentation = spec["documentation"]

    documentation_string += "<h2>/api/" + spec["REST_func"] + "</h2>\n\n"

    try:
        documentation_string += "<b>Purpose</b>: " + documentation["purpose"] + "<br><br>\n\n"
    except KeyError:
        pass

    if "json_output" in documentation:
        documentation["json_output"]["wrench_api_request_success"] = ["bool", "true if success, false if failure"]
        documentation["json_output"]["failure_cause"] = ["string", "human-readable error message (if failure)"]

    for json_io in ["input", "output"]:
        documentation_string += "<b>JSON " + json_io + ":</b> "

        try:
            json_items = documentation["json_" + json_io]
        except KeyError:
            continue

        if len(json_items) == 0:
            documentation_string += "{} (empty JSON)<br><br>\n"
            continue

        documentation_string += "<br>\n<table border=1>\n\n"
        documentation_string += "<tr><th>name/key</th><th>type</th><th>meaning</th></tr>\n"
        for item in json_items:
            documentation_string += "<tr><td style=\"padding:5px\">  \"" + item + "\"</td><td style=\"padding:5px\"> " + json_items[item][0] + "</td><td style=\"padding:5px\">" + json_items[item][1] + " </td></tr>\n"

        documentation_string += "\n\n"
        documentation_string += "</table><br>"

    return documentation_string


def generate_documentation(json_specs):

    documentation = ""

    for spec in json_specs:
        documentation += generate_documentation_item(spec)

    documentation += "<hr>\n"

    return documentation


def grab_json_comments(cpp_source_file):

    try:
        cpp_source_lines = open(cpp_source_file).readlines()
    except IOError:
        raise Exception("Can't read file " + cpp_source_file)

    all_specs = []
    phase = 0 # 0: haven't found BEGIN yet; 1: haven't found END yet; 2: haven't found Method yet
    spec_string = ""
    line_number = 0
    for line in cpp_source_lines:
        line_number += 1
        if line.find("BEGIN_REST_API_DOCUMENTATION") != -1:
            phase = 1
            begin_line_number = line_number
        elif line.find("END_REST_API_DOCUMENTATION") != -1:
            phase = 2
            end_line_number = line_number
        elif phase == 1:
            line = re.sub(".*\*", "", line.strip())
            spec_string += line
        elif phase == 2:
            if line.find("::") != -1:
                full_method_name = re.sub(".* ([a-zA-Z0-9]+)::([a-zA-Z0-9]+)\(.*", "\\1::\\2", line.strip())
                method_name = re.sub(".*::", "", full_method_name)
                try:
                    json_spec = json.loads(spec_string)
                except:
                    raise Exception("Syntax error in JSON documentation of method " + full_method_name +
                                    "() (lines " + str(begin_line_number) + "-" + str(end_line_number) + ")")

                if line.find("SimulationController::") != -1:
                    json_spec["controller_method"] = method_name

                json_spec["wrench_api_request_success"] = ["bool", "True is success, false if failure"]
                json_spec["failure_cause"] = ["string", "Human-readable failure cause message (if failure)"]
                all_specs.append(json_spec)
                spec_string = ""
                phase = 0

    if phase == 1:
        raise Exception(cpp_source_file + ": BEGIN_REST_API_DOCUMENTATION at line " + str(begin_line_number) + " has no matching END_REST_API_DOCUMENTATION")
    elif phase == 2:
        raise Exception(cpp_source_file + ": END_REST_API_DOCUMENTATION at line " + str(end_line_number) + " is not followed by a method definition")

    return all_specs


def construct_json_specs(src_path):
    specs = []
    import glob
    cpp_source_files = glob.glob(src_path+"/*.cpp", recursive=True)
    for cpp_source_file in cpp_source_files:

        try:
            specs += grab_json_comments(cpp_source_file)
        except Exception as e:
            sys.stderr.write("Error while processing file " + cpp_source_file + ": " + str(e) + "\n")
            sys.exit(1)

    return specs


if __name__ == "__main__":

    if len(sys.argv) < 3:
        sys.stderr.write("Usage: " + sys.argv[0] + " <include path> <src path> [<doc file path>]\n")
        sys.exit(0)

    # Grab all the JSON from the .cpp comments
    json_specs = construct_json_specs(sys.argv[2])

    generated_code = ""
    for spec in json_specs:
        if "controller_method" in spec:
            generated_code += """\trequest_handlers[\"""" + spec["REST_func"] + """\"] = [sc](json data) { return sc->""" + spec["controller_method"] + """(std::move(data)); };\n"""

    # Write generated source code file
    try:
        filename = sys.argv[1] + "/REST_API_generated_code.h"
        open(filename, "w").write(generated_code)
    except IOError:
        sys.stderr.write("Can't write file " + filename)
        sys.exit(1)

    # Write doc file if needed
    if len(sys.argv) == 4:
        try:
            open(sys.argv[3], "w").write(generate_documentation(json_specs))
        except IOError:
            sys.stderr.write("Can't write file " + sys.argv[3])
            sys.exit(1)

    sys.exit(0)

