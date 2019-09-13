from snapcraft.plugins import nil
import subprocess

class Ppa(nil.NilPlugin):

    @classmethod
    def schema(cls):
        schema = super().schema()
        schema['properties']['ppa'] = { 'type': 'array', 'items': { 'type': 'string' }}
        schema['required'] = ['ppa']
        return schema


    def __init__(self, name, options, project):
        super().__init__(name, options, project)
        subprocess.run(["sudo", "apt", "--assume-yes", "install", "software-properties-common"])
        for ppa in self.options.ppa:
            subprocess.run(["sudo", "add-apt-repository", "-yu", "ppa:"+ppa])
