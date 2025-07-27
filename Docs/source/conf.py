# Configuration file for the Sphinx documentation builder.

project = 'Engine'
copyright = '2025, Yu-Wei Tseng'
author = 'Yu-Wei Tseng'
release = '1.0.0'

extensions = [
    'sphinx_rtd_theme',
]

templates_path = []
exclude_patterns = []

html_theme = 'sphinx_rtd_theme'
html_static_path = []

# 確保 master document 設定正確
master_doc = 'index'