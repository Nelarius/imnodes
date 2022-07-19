# Sonos
import sonos.audio.dynamicdsp
import sonos.audio.dynamicdsp.commands
import sonos.coreaudio.dsp.v1alpha1

def result():
    api_version = sonos.audio.dynamicdsp.ApiVersion.v1alpha1
    list_output = sonos.audio.dynamicdsp.commands.list(api_version)
    print(list_output)
    return 5