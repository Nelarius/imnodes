# Sonos
import sonos.audio.dynamicdsp
import sonos.audio.dynamicdsp.commands
import sonos.coreaudio.dsp.v1alpha1

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