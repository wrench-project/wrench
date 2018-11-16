<img src="public/logo-vertical.png" width="100" />

# Wrench Dashboard 

Welcome to the Wrench Dashboard! This is where you can fulfill all of your data visualisation needs.

## Required Dependancies

- [Node.js](https://nodejs.org/en/)
- [npm](https://www.npmjs.com/)

## Initial Setup

1. Intall the above dependancies
2. Run `npm install` in this directory

## Instructions to run

The dashboard works by parsing a json dump file from a simulation and outputting graphs onto a local HTML file that you can then view in your browser. Here are the steps to run the program.

Whenever you want to read data from a new JSON file or your current JSON file has been updated, run `node parser.js path/to/json/file`. This will automatically open the HTML file in your default browser. The HTML file is called `index.html` and is located in this directory, you can open it at any time in your browser of choice.

If you've just cloned the repo or done a `git pull` and you haven't run `node parser.js path/to/json/file` yet and you're trying to view the `index.html` file, don't be alarmed if no graph shows up, the graph will only show up once you run the aforementioned command.
