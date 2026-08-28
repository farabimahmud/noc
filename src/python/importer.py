# Copyright (c) 2008 The Hewlett-Packard Development Company
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function
from __future__ import absolute_import

# Simple importer that allows python to import data from a dict of
# code objects.  The keys are the module path, and the items are the
# filename and bytecode of the file.
class CodeImporter(object):
    '''Implemented against the PEP 451 finder/loader protocol
    (find_spec/exec_module): Python 3.12 dropped support for the older
    PEP 302 find_module/load_module protocol this used to use.'''
    def __init__(self):
        self.modules = {}

    def add_module(self, filename, abspath, modpath, code):
        if modpath in self.modules:
            raise AttributeError("%s already found in importer" % modpath)

        self.modules[modpath] = (filename, abspath, code)

    def _is_package(self, fullname):
        import os
        srcfile, abspath, code = self.modules[fullname]
        return os.path.basename(srcfile) == '__init__.py'

    def find_spec(self, fullname, path, target=None):
        if fullname not in self.modules:
            return None

        import importlib.util
        return importlib.util.spec_from_loader(
            fullname, self, is_package=self._is_package(fullname))

    def create_module(self, spec):
        return None

    def exec_module(self, mod):
        # Because the importer is created and initialized in its own
        # little sandbox (in init.cc), the globals that were available
        # when the importer module was loaded and CodeImporter was
        # defined are not available when exec_module is actually
        # called. Soooo, the imports must live here.
        import os
        import sys

        fullname = mod.__name__
        try:
            mod.__loader__ = self
            srcfile,abspath,code = self.modules[fullname]

            override = os.environ.get('M5_OVERRIDE_PY_SOURCE', 'false').lower()
            if override in ('true', 'yes') and  os.path.exists(abspath):
                src = open(abspath, 'r').read()
                code = compile(src, abspath, 'exec')

            if os.path.basename(srcfile) == '__init__.py':
                mod.__path__ = fullname.split('.')
                mod.__package__ = fullname
            else:
                mod.__package__ = fullname.rpartition('.')[0]
            mod.__file__ = srcfile

            exec(code, mod.__dict__)
        except Exception:
            del sys.modules[fullname]
            raise

# Create an importer and add it to the meta_path so future imports can
# use it.  There's currently nothing in the importer, but calls to
# add_module can be used to add code.
import sys
importer = CodeImporter()
add_module = importer.add_module
sys.meta_path.append(importer)
