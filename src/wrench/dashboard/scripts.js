var data={"modified":"2018-11-11T08:58:37.419Z","file":"test_data.json","contents":[{"compute":{"end":6,"start":3},"execution_host":"Host1","failed":2,"num_cores_allocated":1,"read":{"end":-1,"start":0},"task_id":"ID00000","terminated":-1,"whole_task":{"end":9,"start":0},"write":{"end":9,"start":6}},{"compute":{"end":-1,"start":13},"execution_host":"Host1","failed":-1,"num_cores_allocated":1,"read":{"end":13,"start":10},"task_id":"ID00001","terminated":14,"whole_task":{"end":19,"start":10},"write":{"end":19,"start":16}},{"compute":{"end":26,"start":23},"execution_host":"Host1","failed":-1,"num_cores_allocated":1,"read":{"end":23,"start":20},"task_id":"ID00002","terminated":28,"whole_task":{"end":29,"start":20},"write":{"end":-1,"start":26}}]}

/**
 * Helper function used to get the position of the mouse within the browser window
 * so that we can have nice moving tooltips. The input is the DOM element we are
 * interested in (in this case the #chart element).
 */
function getOffset(el, position) {
    const rect = el.getBoundingClientRect();
    return {
        left: rect.left + position[0],
        top: rect.top + position[1]
    };
}

function findDuration(data, id, section) {
    for (var i = 0; i < data.length; i++) {
        var currData = data[i]
        if (currData.task_id == id) {
            if (currData[section].end == -1) {
                if (currData.terminated == -1) {
                    return currData.failed - currData[section].start
                } else if (currData.failed == -1) {
                    return currData.terminated - currData[section].start
                }
            }
            return currData[section].end - currData[section].start
        }
    }
}

function getBoxWidth(d, section, scale) {
    if (d[section].end == -1) {
        if (d.terminated != -1) {
            return scale(d.terminated) - scale(d[section].start)
        } else if (d.failed != -1){
            return scale(d.failed) - scale(d[section].start)
        }
    } else {
        return scale(d[section].end) - scale(d[section].start)
    }
}

function determineFailedOrTerminatedPoint(d) {
    if (d.failed == -1 && d.terminated == -1) {
        return "none"
    }
    if (d.read.end == -1) {
        return "read"
    }
    if (d.compute.end == -1) {
        return "compute"
    }
    if (d.write.end == -1) {
        return "write"
    }
}

function generateGraph() {
    var modified = data.modified
    var file = data.file
    document.getElementById('updated').innerHTML = `Data from file "${file}" was last updated on ${new Date(modified)}`
    data = data.contents
    var read_color    = '#cbb5dd';
    var compute_color = '#f7daad';
    var write_color   = '#abdcf4';
    var container = d3.select("#graph-container");
    const CONTAINER_WIDTH = container.style("width").slice(0, -2); // returns "XXXXpx" so need to remove "px"
    const CONTAINER_HEIGHT = container.style("height").slice(0, -2);
    const PADDING = 60;
    var toFiveDecimalPlaces = d3.format('.5f');
    var svg = container
        .append("svg")
        .attr('width', CONTAINER_WIDTH)
        .attr('height', CONTAINER_HEIGHT)
        .attr('id', 'graph')
    var xscale = d3.scaleLinear()
        .domain([0, d3.max(data, function(d) {
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
        var readTime = getBoxWidth(d, "read", xscale)
        var computeTime = getBoxWidth(d, "compute", xscale)
        var writeTime = getBoxWidth(d, "write", xscale)
        var ft_point = determineFailedOrTerminatedPoint(d)
        if (ft_point != "none") {
            var x_ft = xscale(d.terminated == -1 ? d.failed : d.terminated)
            var full_height_ft = yscale(data[0].task_id)-yscale(data[1].task_id)
            var height_ft = full_height_ft/15
            var y_ft = yscale(d.task_id) + (full_height_ft/2) - height_ft/2
            var colour_ft = d.terminated == -1 ? 'orange' : 'red'
            var class_ft = d.terminated == -1 ? 'failed' : 'terminated'
            group.append('rect')
                .attr('x', x_ft)
                .attr('y', y_ft)
                .attr('height', height_ft)
                .attr('width', xscale(d.whole_task.end) - xscale(d.terminated == -1 ? d.failed : d.terminated))
                .style('fill', colour_ft)
                .attr('class', class_ft)
        }
        group.append('rect')
            .attr('x', xscale(d.read.start))
            .attr('y', yscale(d.task_id))
            .attr('height', yscale(data[0].task_id)-yscale(data[1].task_id))
            .attr('width', readTime)
            .style('fill',read_color)
            .attr('class','read')
        if (ft_point != "read" || ft_point == "none") {
            group.append('rect')
                .attr('x', xscale(d.compute.start))
                .attr('y', yscale(d.task_id))
                .attr('height', yscale(data[0].task_id)-yscale(data[1].task_id))
                .attr('width', computeTime)
                .style('fill',compute_color)
                .attr('class','compute')
        }
        if ((ft_point != "read" && ft_point != "compute" )|| ft_point == "none") {
            group.append('rect')
                .attr('x', xscale(d.write.start))
                .attr('y', yscale(d.task_id))
                .attr('height', yscale(data[0].task_id)-yscale(data[1].task_id))
                .attr('width', writeTime)
                .style('fill',write_color)
                .attr('class','write')
        }
        // var tooltip = d3.select('tooltip-container')
        var tooltip = document.getElementById('tooltip-container')
        var tooltip_task_id                 = d3.select('#tooltip-task-id');
        var tooltip_task_operation          = d3.select('#tooltip-task-operation');
        var tooltip_task_operation_duration = d3.select('#tooltip-task-operation-duration');
        group.selectAll('rect')
            .on('mouseover', function() {
                // tooltip.style('display', 'inline');
                tooltip.style.display = 'inline'

                d3.select(this)
                    .attr('stroke', 'gray')
                    .style('stroke-width', '1');
            })
            .on('mousemove', function() {
                var offset = getOffset(document.getElementById("graph-container"), d3.mouse(this));
                var x = window.scrollX + offset.left + 20;
                var y = window.scrollY + offset.top - 30; // 20 seems to account for the padding of the chart-block
                
                tooltip.style.left = x + 'px'
                tooltip.style.top = y + 'px'

                tooltip_task_id.text('TaskID: ' + d.task_id);

                var parent_group = d3.select(this).attr('class');

                if (parent_group == 'read') {
                    tooltip_task_operation.text('Read Input');
                } else if (parent_group == 'compute') {
                    tooltip_task_operation.text('Computation');
                } else if (parent_group == 'write') {
                    tooltip_task_operation.text('Write Output');
                } else if (parent_group == "terminated") {
                    tooltip_task_operation.text('Terminated');
                } else if (parent_group == "failed") {
                    tooltip_task_operation.text('Failed');
                }

                var durationFull = findDuration(data, d.task_id, parent_group)
                if (parent_group != "failed" && parent_group != "terminated") {
                    var duration = toFiveDecimalPlaces(durationFull);
                    tooltip_task_operation_duration.text('Duration: ' + duration + 's');
                } else {
                    tooltip_task_operation_duration.text('');
                }
                
            })
            .on('mouseout', function() {
                tooltip.style.display = 'none'

                d3.select(this)
                    .attr('stroke', 'none')
            });
    })
}