import SCons
from SCons.Builder import Builder
from SCons.Environment import Environment
from SCons.Action import Action

from typing import Any

import pathlib

import jinja2
from jinja2 import Template

import toml


def load_config_from_file(file: str) -> dict[str, Any]:
    ext = pathlib.Path(file).suffix
    match ext:
        case ".toml":
            return toml.load(file)
        case _:
            raise ValueError(f"unknown extension {ext}")


def store_config_to_file(conf: dict[str, Any], file: str) -> None:
    ext = pathlib.Path(file).suffix
    match ext:
        case ".toml":
            with open(file, "w") as f:
                toml.dump(conf, f)
        case _:
            raise ValueError(f"unknown extension {ext}")


def jinja_configure(target, source, env: Environment):
    template: Template
    with open(str(source[0]), "rt") as in_template:
        template = Template(in_template.read(), keep_trailing_newline=True)
    config = load_config_from_file(str(source[1]))
    with open(str(target[0]), "wt") as outfile:
        outfile.write(template.render(config))
    return 0


def preprocess_isa(target, source, env: Environment):
    isa_description = load_config_from_file(str(source[0]))
    instructions: list[dict[str, Any]] = isa_description["instructions"]

    custom_instructions: list[int] = []
    instruction_classes = [{"name": "custom", "list": custom_instructions}]
    isa_description["classes"] = instruction_classes

    for index, inst in enumerate(instructions):
        inst["index"] = index
        custom_instructions.append(index)

    current_op_code = 1
    for inst_class in instruction_classes:
        inst_list: list[int] = inst_class["list"]
        for inst_indx in inst_list:
            inst = instructions[inst_indx]
            if not inst.get("pseudo", False):
                inst["opcode"] = current_op_code
                current_op_code += 1

    store_config_to_file(isa_description, str(target[0]))
    return 0


def exists(env: Environment) -> bool:
    return True


def generate(env: Environment):
    env.Append(
        BUILDERS={
            "JinjaConfigure": Builder(action=jinja_configure),
            "IsaPreprocess": Builder(action=preprocess_isa),
        }
    )
