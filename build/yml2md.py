#!/usr/bin/python3

import argparse
import os


def read_file(filename):
    with open(filename, "rb") as fh:
        return fh.read()


def write_file(filename, data):
    with open(filename, "wb") as fh:
        fh.write(data.encode("utf-8"))


def load_yaml(input):
    f = read_file(input)
    try:
        return yaml.safe_load(f)
    except yaml.YAMLError:
        return []


def generate_variables(job):
    if "variables" not in job:
        return ""

    return """
Variables:
```bash
{variables}```""".format(variables=job["variables"])


def generate_env_variables(variables):
    output = """
# Environment variables
```bash
"""
    for key, value in variables.items():
        output += "{var} = {value}\n".format(var=key, value=value)
    output += "```"
    return output


def generate_jobs(jobs):
    output = ""
    for key in sorted(jobs.keys()):
        value = jobs[key]
        output += """
## Job: {job_name}
Tags:
{tags}
{variables}
Commands:
```bash
{commands}```
""".format(job_name=key, tags=value["tags"],
           commands=value["commands"], variables=generate_variables(value))
    return output


def generate_stage(stage_title, jobs):
    return """
# Stage: {stage_title}
{jobs}
""".format(stage_title=stage_title,
           jobs=generate_jobs(jobs))


def parse_job(job, raw_job):
    if "tags" in raw_job and len(raw_job["tags"]) != 0:
        for t in raw_job["tags"]:
            job["tags"] += "* " + t + "\n"

    if "before_script" in raw_job and len(raw_job["before_script"]) != 0:
        job["commands"] = ""
        for c in raw_job["before_script"]:
            job["commands"] += c + "\n"

    if "script" in raw_job and len(raw_job["script"]) != 0:
        for c in raw_job["script"]:
            job["commands"] += c + "\n"

    stage_name = ""
    if "stage" in raw_job:
        stage_name = raw_job["stage"]

    if "variables" in raw_job:
        if "variables" not in job:
            job["variables"] = ""
        for k, v in raw_job["variables"].items():
            job["variables"] += "%s = %s\n" % (k, v)

    return stage_name


if __name__ == '__main__':
    try:
        import yaml
    except ImportError:
        print("python3-yaml not installed")
        exit(0)

    parser = argparse.ArgumentParser(
        description='generate doc from gitlab-ci.yml')
    parser.add_argument('-i', '--input', type=os.path.abspath,
                        help='input yaml', required=True)
    parser.add_argument('-o', '--output', type=os.path.abspath,
                        help='output md',  default="BUILD.gen.md")
    opts = parser.parse_args()

    raw = load_yaml(opts.input)
    if raw == []:
        print("unable to load yaml")
        exit(1)

    if "stages" not in raw and (not isinstance(raw["stages"], list) or
                                len(raw["stages"]) == 0):
        print("unable to read stages in yaml file")
        exit(1)

    stages = {}
    for s in raw["stages"]:
        stages[s] = {}
    del raw["stages"]

    env_variables = {}
    if "variables" in raw:
        env_variables = raw["variables"]
        del raw["variables"]

    for key, value in raw.items():
        # Hidden key/job
        if key[0] == ".":
            continue

        job = {"tags": "", "commands": ""}
        stage_name = ""

        if "extends" in value and value["extends"] in raw:
            parent = raw[value["extends"]]
            stage_name = parse_job(job, parent)

        s = parse_job(job, value)
        if s != "":
            stage_name = s

        if stage_name in stages:
            tab_key = key.split(":")
            if stage_name in tab_key:
                tab_key.remove(stage_name)

            stages[stage_name][" ".join(tab_key)] = job

    output = """**This file was generated from the .gitlab-ci.yml file that is
                used for continuous integration.**<br>**It contains useful
                commands to build the project.**<br>"""
    output += generate_env_variables(env_variables)

    for key, value in stages.items():
        output += generate_stage(key, value)

    write_file(opts.output, output)
