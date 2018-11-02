const express = require('express');
const app = express();
const bodyParser = require('body-parser');
const fs = require("fs");
const d3 = require("d3");
const jsdom = require("jsdom");
const { JSDOM } = jsdom;
app.use( bodyParser.json() );       // to support JSON-encoded bodies
app.use(bodyParser.urlencoded({     // to support URL-encoded bodies
  extended: true
}));

function parseFile(path) {
    return new Promise(function(resolve, reject) {
        fs.readFile(path, 'utf8', function(err, contents) {
            if (err) {
                reject({err});
            } else {
                resolve(JSON.parse(contents));
            }
        });
    });
}

function generateGraph(data) {
    const html = `
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8" />
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <title>Dashboard</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <link rel="stylesheet" href="bootstrap.min.css">
        <!-- <script src="bootstrap.min.js" type="text/javascript"></script> -->
    </head>
    <body>
        <div id="container" style="height:1000px; width:1000px">

        </div>
    </body>
    </html>    
    `
    const dom = new JSDOM(html)
    var document = d3.select(dom.window.document)
    var read_color    = '#cbb5dd';
    var compute_color = '#f7daad';
    var write_color   = '#abdcf4';
    var container = document.select("#container");
    const CONTAINER_WIDTH = container.style("width").slice(0, -2); // returns "XXXXpx" so need to remove "px"
    const CONTAINER_HEIGHT = container.style("height").slice(0, -2);
    const PADDING = 60;
    var svg = container
        .append("svg:svg")
        .attr('width', CONTAINER_WIDTH)
        .attr('height', CONTAINER_HEIGHT);
        console.log(d3.max(data, function(d) {
            return d['whole_task'].end;
           
        }));
    var xscale = d3.scaleLinear()
        .domain([0,d3.max(data, function(d) {
            return d['whole_task'].end;
           
        })])
        .range([PADDING, CONTAINER_WIDTH - PADDING]);

    var task_ids = [];
    data.forEach(function(task) {
        task_ids.push(task['task_id']);
    });

    task_ids.sort(function(lhs, rhs) {
       return parseInt(lhs.slice(4)) - parseInt(rhs.slice(4));
    });

    var yscale = d3.scaleBand()
        .domain(task_ids)
        .range([CONTAINER_HEIGHT - PADDING, 30])
        .padding(0.1);

    var x_axis = d3.axisBottom()
        .scale(xscale);

    var y_axis = d3.axisLeft()
        .scale(yscale);

    svg.append('g')
        .attr('class', 'y-axis')
        .attr('transform',
            'translate(' + PADDING + ',0)')
        .call(y_axis);

    svg.append('g')
        .attr('class', 'x-axis')
        .attr('transform',
            'translate(0,' + (CONTAINER_HEIGHT - PADDING) + ')')
        .call(x_axis);
    data.forEach(function(d) {
        var group = svg.append('g')
           .attr('id', d.task_id)
        group.append('rect')
                .attr('x', xscale(d.read.start))
                .attr('y', yscale(d.task_id))
                .attr('height', yscale(data[0].task_id)-yscale(data[1].task_id))
                .attr('width', xscale(d.read.end) - xscale(d.read.start))
                .style('fill',read_color)
        group.append('rect')
                .attr('x', xscale(d.compute.start))
                .attr('y', yscale(d.task_id))
                .attr('height', yscale(data[0].task_id)-yscale(data[1].task_id))
                .attr('width', xscale(d.compute.end) - xscale(d.compute.start))
                .style('fill',compute_color)
        group.append('rect')
                .attr('x', xscale(d.write.start))
                .attr('y', yscale(d.task_id))
                .attr('height', yscale(data[0].task_id)-yscale(data[1].task_id))
                .attr('width', xscale(d.write.end) - xscale(d.write.start))
                .style('fill',write_color)
    })
    return dom;
}

app.get('/generate', async function(req, res) {
    parseFile("gautam.json")
        .then(function(content) {
            var data = JSON.parse(JSON.stringify(content));
            var dom = generateGraph(data);
            res.send(dom.serialize());
        })
        .catch(function(err) {
            res.status(500).json({err})
        })
    // fs.writeFile('index.html', topHalfOfFile, function(){

    // })
});

const port = process.env.PORT || 5000;
app.listen(port, function() {
  console.log(`Listening on port ${port}`);
});