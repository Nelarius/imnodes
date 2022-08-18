# Standard Library
from typing import List, TextIO, cast
import pathlib
import sys

# Third Party
import betterproto
import click
import colorama
from loguru import logger

# Sonos
import sonos.audio.dynamicdsp
import sonos.audio.dynamicdsp.commands
import sonos.coreaudio.dsp.v1alpha1 as api

# Global variables
api_version = sonos.audio.dynamicdsp.ApiVersion.v1alpha1

# Config details
password = None
identity_file = None
passphrase = None
reboot = True
no_reboot = not reboot

# - Helper Function
def _print_error(error_msg: str) -> None:
    """Print a standard error message using click.echo."""
    click.echo(colorama.Fore.RED + error_msg + colorama.Style.RESET_ALL)

def _print_warning(error_msg: str) -> None:
    """Print a warning message using click.echo."""
    click.echo(colorama.Fore.YELLOW + error_msg + colorama.Style.RESET_ALL)

# - Other functions

def list_dsp():
    list_output = sonos.audio.dynamicdsp.commands.list(api_version)
    print(sorted(list_output.dsp_block_names))
    return sorted(list_output.dsp_block_names)

def list_control():
    list_output = sonos.audio.dynamicdsp.commands.list(api_version)
    print(sorted(list_output.control_block_names))
    return sorted(list_output.control_block_names)

def list_params(block_name):
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

def validate(): 
    with open("system.json", 'r') as jf:
        # validate each input file
        did_find_errors = False
        click.echo("Validating {}...".format(jf.name))
        rule_outputs = sonos.audio.dynamicdsp.commands.validate(api_version, jf)

        # check the rule outputs
        is_valid = all(rule_outputs)
        if not is_valid:
            did_find_errors = True
            rules_broken = [r for r in rule_outputs if not r]
            logger.info("Found {} errors in {}", len(rules_broken), jf.name)
            for rule in rules_broken:
                _print_error(rule.message)

        if did_find_errors:
            sys.exit(1)
        #if not did_find_errors:
        click.echo("File {} looks good! \U00002728 \U0001F9C1 \U00002728".format(jf.name))

# TO DO : add error statements, possibly toast notifications in app
def generate_bin():
    validate()
    with open("system.json", 'r') as jf:
        # figure out file paths and names
        default_output_file = pathlib.Path(jf.name).with_suffix(".bin").name
        # - if no output path was specified, we put the file in the current directory
        output_file = pathlib.Path(default_output_file)

        # generate the message binary from the given json file
        click.echo("Generating DSP API message from {}...".format(jf.name))
        raw_bytes = sonos.audio.dynamicdsp.commands.generate(api_version, jf)

        # write the output file
        click.echo("Writing DSP API message to {}...".format(output_file))
        with open(output_file, mode="wb") as bin_file:
            bin_file.write(raw_bytes)
    
    click.echo("DSP System{} generated! \U00002728 \U0001F9C1 \U00002728")


def deploy(ip_address):
    with open("system.bin", 'r') as bf:
        click.echo("Deploying {} to device {}...".format(bf.name, ip_address))

        config = sonos.audio.dynamicdsp.SSHConfig(
            ip_address, password=password, identity_file=identity_file, passphrase=passphrase
        )
        result = sonos.audio.dynamicdsp.commands.deploy(bf, config, api_version, not no_reboot)

        if not result.success:
            _print_error("Failed to deploy {} to device {}".format(bf.name, ip_address))
            for msg in result.error_msgs:
                _print_error(msg)
            for msg in result.warning_msgs:
                _print_warning(msg)
            sys.exit(1)

    click.echo("DSP System deployed to {}! \U00002728 \U0001F9C1 \U00002728".format(ip_address))

def clean(ip_address):
    click.echo("Removing DSP System from device {}...".format(ip_address))
    config = sonos.audio.dynamicdsp.SSHConfig(
        ip_address, password=password, identity_file=identity_file, passphrase=passphrase
    )
    result = sonos.audio.dynamicdsp.commands.clean(config, not no_reboot)
    if not result:
        _print_error("Failed to clean device {}".format(ip_address))
        sys.exit(1)

    click.echo("All set! \U00002728 \U0001F9C1 \U00002728")