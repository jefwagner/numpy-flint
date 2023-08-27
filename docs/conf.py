import os
import sys

sys.path.insert(0, os.path.abspath('../src'))

import flint

version = flint.__version__
release = flint.__version__

project = 'flint'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.todo',
    'sphinx-prompt',
]

todo_include_todos = True

source_suffix = {
    '.rst':'restructuredtext',
    '.md':'markdown',
}

master_doc = 'index'
language = 'en'

html_theme = 'furo'

add_module_names = False
autodoc_typehints = 'none'
