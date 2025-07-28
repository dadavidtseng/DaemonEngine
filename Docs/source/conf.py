# Configuration file for the Sphinx documentation builder.

project = 'DaemonEngine'
copyright = '2025, Yu-Wei Tseng'
author = 'Yu-Wei Tseng'
release = '1.0.0'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx_rtd_theme',
    'breathe',
]

templates_path = []
exclude_patterns = []

html_theme = 'sphinx_rtd_theme'
html_static_path = []

# 確保 master document 設定正確
master_doc = 'index'

breathe_projects = {
    "Engine": "../doxygen/xml"
}
breathe_default_project = "Engine"