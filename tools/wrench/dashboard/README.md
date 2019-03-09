<img src="public/logo-vertical.png" width="100" />

# Wrench Dashboard 

Welcome to the Wrench Dashboard! This is where you can fulfill all of your data visualisation needs.

## Required Dependancies

- [Docker](https://docs.docker.com/install/)

## Initial Setup

1. Install Docker
2. Give the graphing script executable permission by running 'chmod +x createGraph.sh' in this directory

## Instructions to run

The dashboard works by parsing a JSON dump file from a simulation and creating graphs in local HTML file that you can then view in your browser.

To populate the HTML file with the data from the simulation just run the graphing script with './createGraph.sh <file path to JSON dump file>'

The resulting HTML file is called `index.html` and is located in this directory, you can open it at any time in your browser to view the graphs.

If you've just cloned the repo or done a `git pull` and you haven't gone through the setup instructions yet and you're trying to view the `index.html` file, don't be alarmed if no graph shows up, the graph will only show up once you run the script above.
