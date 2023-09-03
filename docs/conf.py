import os
import sys
import sysconfig

sys.path.insert(0, os.path.abspath('../src'))

import flint

version = flint.__version__
release = flint.__version__

project = 'flint'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.todo',
    'sphinx-prompt',
    'hawkmoth'
]

todo_include_todos = True

source_suffix = {
    '.rst':'restructuredtext',
    '.md':'markdown',
}

hawkmoth_root = os.path.abspath(os.path.abspath('../src/flint'))
hawkmoth_clang = ['-I'+sysconfig.get_config_var('INCLUDEPY')]

master_doc = 'index'
language = 'en'

html_theme = 'furo'

add_module_names = False
autodoc_typehints = 'none'
autodoc_member_order = 'bysource'
