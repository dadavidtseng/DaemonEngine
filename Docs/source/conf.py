# Configuration file for the Sphinx documentation builder.

project = 'DaemonEngine'
copyright = '2025, Yu-Wei Tseng'
author = 'Yu-Wei Tseng'
release = '1.0.0'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx_rtd_theme',
    'breathe',
]

templates_path = ['_templates']
exclude_patterns = []

# -- Options for HTML output ------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Theme options
html_theme_options = {
    'logo_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'style_nav_header_background': '#2c3e50',
    'collapse_navigation': False,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# Logo and branding
html_logo = '_static/images/daemon-engine-logo.svg'
html_favicon = '_static/images/daemon-engine-icon.ico'

# Custom CSS and JS
html_css_files = [
    'css/custom.css',
]

html_js_files = [
    'js/custom.js',
]

# Site info
html_title = 'DaemonEngine Documentation'
html_short_title = 'DaemonEngine'
html_show_sourcelink = True
html_show_sphinx = False
html_show_copyright = True

# Master document
master_doc = 'index'

# -- Breathe configuration --------------------------------------------------

breathe_projects = {
    "DaemonEngine": "../doxygen/xml"
}
breathe_default_project = "DaemonEngine"

# -- Extension configuration -------------------------------------------------

# Todo extension
todo_include_todos = True

# Intersphinx mapping
intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
}