#!/usr/bin/python3

import sphinx_rtd_theme
import sys

# -- Project information -----------------------------------------------------

project = 'WRENCH'
copyright = '2017-2022, WRENCH'
author = 'WRENCH Team'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'recommonmark',
    'sphinx_rtd_theme',
    'sphinx.ext.autodoc',
    'sphinx.ext.doctest',
    'sphinx.ext.mathjax',
    'sphinx.ext.viewcode',
    'sphinx.ext.imgmath',
    'sphinx.ext.todo',
    'breathe',
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
source_suffix = {".rst": "restructuredtext", ".md": "markdown"}

# The master toctree document.
master_doc = "index"

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "_*.rst"]


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'
html_favicon = 'favicon.png'

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_logo = "images/wrench-dark-theme-logo.png"
html_css_files = [
    'css/custom.css',
]
html_theme_options = {
    'logo_only': True,
    'display_version': True,
}

# -- Extension configuration -------------------------------------------------

breathe_default_project = "user"

# The full version, including alpha/beta/rc tags
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
breathe_projects = {
    "user": "../../docs/2.1-dev/user/xml/",
    "developer": "../../docs/2.1-dev/developer/xml/",
    "internal": "../../docs/2.1-dev/internal/xml/",
}

version = '2.1-dev'
release = '2.1-dev'
