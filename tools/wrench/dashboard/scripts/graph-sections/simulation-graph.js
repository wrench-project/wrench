function getBoxWidth(d, section, scale) {
    if (d[section].start != -1) {
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
    return scale(0) //Box shouldn't be displayed if start is -1
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

/**
 * Helper function used to get the position of the mouse within the browser window
 * so that we can have nice moving tooltips. The input is the DOM element we are
 * interested in (in this case the #chart element).
 */
function getOffset(el, position) {
    const rect = el.getBoundingClientRect()
    return {
        left: rect.left + position[0],
        top: rect.top + position[1]
    }
}

/*
    data: simulation data,
    containerId: id of <div> container of graph
    currGraphState: pass in "hostView" to see the host view and "taskView" to see the task view
*/
function generateGraph(data, containerId, currGraphState, CONTAINER_WIDTH, CONTAINER_HEIGHT) {
    document.getElementById(containerId).innerHTML = //reset graph
        `<div class="text-left" id="tooltip-container">
            <span id="tooltip-task-id"></span><br>
            <span id="tooltip-host"></span><br>
            <span id="tooltip-task-operation"></span><br>
            <span id="tooltip-task-operation-duration"></span>
        </div>`
    
    var read_color    = '#cbb5dd'
    var compute_color = '#f7daad'
    var write_color   = '#abdcf4'
    var container = d3.select(`#${containerId}`)
    const PADDING = 60
    const RANGE_END = 30
    var svg = container
        .append("svg")
        .attr('width', CONTAINER_WIDTH)
        .attr('height', CONTAINER_HEIGHT)
        .attr('id', 'graph')
    var xscale = d3.scaleLinear()
        .domain([0, d3.max(data, function(d) {
            return Math.max(d['whole_task'].end, d['failed'], d['terminated'])
        })])
        .range([PADDING, CONTAINER_WIDTH - PADDING])

    var task_ids = []
    data.forEach(function(task) {
        task_ids.push(task['task_id'])
    })

    task_ids.sort(function(lhs, rhs) {
       return parseInt(lhs.slice(4)) - parseInt(rhs.slice(4))
    })

    var yscale = d3.scaleBand()
        .domain(task_ids)
        .range([CONTAINER_HEIGHT - PADDING, RANGE_END])
        .padding(0.1)

    var x_axis = d3.axisBottom()
        .scale(xscale)

    var y_axis = d3.axisLeft()
        .scale(yscale)

    svg.append('g')
        .attr('class', 'y-axis')
        .attr('transform',
            'translate(' + PADDING + ',0)')
        .call(y_axis)

    svg.append('g')
        .attr('class', 'x-axis')
        .attr('transform',
            'translate(0,' + (CONTAINER_HEIGHT - PADDING) + ')')
        .call(x_axis)

    // text label for the x axis
    svg.append("text")
        .attr("class", "x label")
        .attr("text-anchor", "end")
        .attr("x", CONTAINER_WIDTH/2)
        .attr("y", CONTAINER_HEIGHT - 6)
        .text("Time (seconds)")

    var yAxisText = currGraphState === "taskView" ? "TaskID" : "Host Name"

    // text label for the y axis
    svg.append("text")
        .attr("id", "y-axis-label")
        .attr("transform", "rotate(-90)")
        .attr("y", 0)
        .attr("x",0 - (CONTAINER_HEIGHT / 2))
        .attr("dy", "1em")
        .style("text-anchor", "middle")
        .text(yAxisText)

    data.forEach(function(d) {
        var height = data.length > 1 ? yscale(data[0].task_id)-yscale(data[1].task_id) : CONTAINER_HEIGHT - PADDING - RANGE_END;
        var yScaleNumber = data.length > 1 ? yscale(d.task_id) : RANGE_END
        var group = svg.append('g')
           .attr('id', d.task_id)
        var readTime = getBoxWidth(d, "read", xscale)
        var computeTime = getBoxWidth(d, "compute", xscale)
        var writeTime = getBoxWidth(d, "write", xscale)
        var ft_point = determineFailedOrTerminatedPoint(d)
        if (ft_point != "none") {
            var x_ft = xscale(d.terminated == -1 ? d.failed : d.terminated)
            var colour_ft = d.terminated == -1 ? 'orange' : 'red'
            var class_ft = d.terminated == -1 ? 'failed' : 'terminated'
            group.append('rect')
                .attr('x', x_ft)
                .attr('y', yScaleNumber)
                .attr('height', height)
                .attr('width', '5px')
                .style('fill', colour_ft)
                .attr('class', class_ft)
        }
        group.append('rect')
            .attr('x', xscale(d.read.start))
            .attr('y', yScaleNumber)
            .attr('height', height)
            .attr('width', readTime)
            .style('fill',read_color)
            .attr('class','read')
        if (ft_point != "read" || ft_point == "none") {
            group.append('rect')
                .attr('x', xscale(d.compute.start))
                .attr('y', yScaleNumber)
                .attr('height', height)
                .attr('width', computeTime)
                .style('fill',compute_color)
                .attr('class','compute')
        }
        if ((ft_point != "read" && ft_point != "compute" )|| ft_point == "none") {
            group.append('rect')
                .attr('x', xscale(d.write.start))
                .attr('y', yScaleNumber)
                .attr('height', height)
                .attr('width', writeTime)
                .style('fill',write_color)
                .attr('class','write')
        }
        var tooltip = document.getElementById("tooltip-container")
        var tooltip_task_id                 = d3.select("#tooltip-task-id")
        var tooltip_host                    = d3.select("#tooltip-host")
        var tooltip_task_operation          = d3.select("#tooltip-task-operation")
        var tooltip_task_operation_duration = d3.select("#tooltip-task-operation-duration")
        group.selectAll('rect')
            .on('mouseover', function() {
                tooltip.style.display = 'inline'

                d3.select(this)
                    .attr('stroke', 'gray')
                    .style('stroke-width', '1')
            })
            .on('mousemove', function() {
                var offset = getOffset(document.getElementById(containerId), d3.mouse(this))
                var x = window.scrollX + offset.left + 20
                var y = window.scrollY + offset.top - 30 // 20 seems to account for the padding of the chart-block
                
                tooltip.style.left = x + 'px'
                tooltip.style.top = y + 'px'

                tooltip_task_id.text('TaskID: ' + d.task_id)

                tooltip_host.text('Host Name: ' + d.execution_host.hostname)

                var parent_group = d3.select(this).attr('class')

                if (parent_group == 'read') {
                    tooltip_task_operation.text('Read Input')
                } else if (parent_group == 'compute') {
                    tooltip_task_operation.text('Computation')
                } else if (parent_group == 'write') {
                    tooltip_task_operation.text('Write Output')
                } else if (parent_group == "terminated") {
                    tooltip_task_operation.text('Terminated')
                } else if (parent_group == "failed") {
                    tooltip_task_operation.text('Failed')
                }

                var durationFull = findDuration(data, d.task_id, parent_group)
                if (parent_group != "failed" && parent_group != "terminated") {
                    var duration = toFiveDecimalPlaces(durationFull)
                    tooltip_task_operation_duration.text('Duration: ' + duration + 's')
                } else {
                    tooltip_task_operation_duration.text('')
                }
                
            })
            .on('mouseout', function() {
                tooltip.style.display = 'none'

                d3.select(this)
                    .attr('stroke', 'none')
            })
    })
}