# Standard Library
from typing import List, TextIO, cast

# Third Party
import betterproto
import click
import colorama
from loguru import logger

# Sonos
import sonos.audio.dynamicdsp
import sonos.audio.dynamicdsp.commands
import sonos.coreaudio.dsp.v1alpha1 as api

# - Helper Function
def _print_error(error_msg: str) -> None:
    """Print a standard error message using click.echo."""
    click.echo(colorama.Fore.RED + error_msg + colorama.Style.RESET_ALL)

def list_dsp():
    api_version = sonos.audio.dynamicdsp.ApiVersion.v1alpha1
    list_output = sonos.audio.dynamicdsp.commands.list(api_version)
    print(sorted(list_output.dsp_block_names))
    return sorted(list_output.dsp_block_names)

def list_control():
    api_version = sonos.audio.dynamicdsp.ApiVersion.v1alpha1
    list_output = sonos.audio.dynamicdsp.commands.list(api_version)
    print(sorted(list_output.control_block_names))
    return sorted(list_output.control_block_names)

def list_params(block_name):
    api_version = sonos.audio.dynamicdsp.ApiVersion.v1alpha1
    list_output = sonos.audio.dynamicdsp.commands.describe(api_version, block_name)
    if (not list_output):
        return ["none"]
    print(sorted(list_output))
    return sorted(list_output)

def list_param_types(block_name):
    if api.DspBlockParams()._betterproto.cls_by_field.get(block_name):
        block_class = api.DspBlockParams()._betterproto.cls_by_field.get(block_name)

    elif api.ControlBlockParams()._betterproto.cls_by_field.get(block_name):
        block_class = api.ControlBlockParams()._betterproto.cls_by_field.get(block_name)

    block_param_types = []
    block = block_class()
    for param in block.__dataclass_fields__.values():
        try:
            block_param_types.append(param.type.__name__)
        except:
            block_param_types.append(param.type.__class__.__name__)

    if (not block_param_types):
        return ["none"]
    print(block_param_types)
    return block_param_types


def list_rules(): 
    api_version = sonos.audio.dynamicdsp.ApiVersion.v1alpha1
    f = open("system.json", 'r')


    # validate each input file
    did_find_errors = False
    click.echo("Validating {}...".format(f.name))
    rule_outputs = sonos.audio.dynamicdsp.commands.validate(api_version, f)

    # check the rule outputs
    is_valid = all(rule_outputs)
    if not is_valid:
        did_find_errors = True
        rules_broken = [r for r in rule_outputs if not r]
        logger.info("Found {} errors in {}", len(rules_broken), f.name)
        for rule in rules_broken:
            _print_error(rule.message)

    if not did_find_errors:
        #sys.exit(1)
        click.echo(
            "File{} look{} good! \U00002728 \U0001F9C1 \U00002728"
        )