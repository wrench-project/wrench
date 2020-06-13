function getBoxWidthFromArray(d, section, scale) {
    let dict = {
        "start": scale(0),
        "end": scale(0)
    }
    if (Object.keys(d[section]).length > 0) {
        let min_time = Number.MAX_VALUE;
        let duration = 0;
        for (key in Object.keys(d[section])) {
            let time = getBoxWidth(d[section], key, scale);
            if (time != scale(0)) {
                duration += time;
                min_time = scale(d[section][key].start) < min_time ? scale(d[section][key].start) : min_time;
            }
        }
        dict.start = min_time
        dict.end = duration
    }
    return dict
}

function getBoxWidth(d, section, scale) {
    const timeObj = section === "" ? d : d[section]
    if (timeObj.start != -1) {
        if (timeObj.end == -1) {
            if (d.terminated != -1) {
                return scale(d.terminated) - scale(timeObj.start)
            } else if (d.failed != -1) {
                return scale(d.failed) - scale(timeObj.start)
            }
        } else {
            return scale(timeObj.end) - scale(timeObj.start)
        }
    }
    return scale(0) //Box shouldn't be displayed if start is -1
}

function addBox(group, x, y, height, width, fill, operation, parentGroup) {
    group.append('rect')
        .attr('x', x)
        .attr('y', y)
        .attr('height', height)
        .attr('width', width)
        .style('fill', fill)
        .attr('operation', operation)
        .attr('parentGroup', parentGroup)
}

function addLine(group, x1, y1, x2, y2) {
    group.append('line')
        .attr('x1', x1)
        .attr('y1', y1)
        .attr('x2', x2)
        .attr('y2', y2)
        .style('stroke', 'rgb(0,0,0)')
        .style('stroke-width', 2)
}

/*
    data: simulation data,
    currGraphState: pass in "hostView" to see the host view and "taskView" to see the task view,
    partitionIO: boolean for whether or not to partition the I/O sections of the graph
    CONTAINER_WIDTH: width of the container
    CONTAINER_HEIGHT: height of the container
*/
function generateGraph(data, currGraphState, partitionIO, CONTAINER_WIDTH, CONTAINER_HEIGHT) {
    const containerId = "graph-container"
    document.getElementById(containerId).innerHTML = simulationGraphTooltipHtml
    var read_color = '#cbb5dd'
    var compute_color = '#f7daad'
    var write_color = '#abdcf4'
    var container = d3.select(`#${containerId}`)
    const PADDING = 60
    const RANGE_END = 30
    var svg = container
        .append("svg")
        .attr('width', CONTAINER_WIDTH)
        .attr('height', CONTAINER_HEIGHT)
        .attr('id', 'graph')
    var xscale = d3.scaleLinear()
        .domain([0, d3.max(data, function (d) {
            return Math.max(d['whole_task'].end, d['failed'], d['terminated'])
        })])
        .range([PADDING, CONTAINER_WIDTH - PADDING])

    var task_ids = []
    data.forEach(function (task) {
        task_ids.push(task['task_id'])
    })

    task_ids.sort(function (lhs, rhs) {
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
        .attr("x", CONTAINER_WIDTH / 2)
        .attr("y", CONTAINER_HEIGHT - 6)
        .text("Time (seconds)")

    var yAxisText = currGraphState === "taskView" ? "TaskID" : "Host Name"

    // text label for the y axis
    svg.append("text")
        .attr("id", "y-axis-label")
        .attr("transform", "rotate(-90)")
        .attr("y", 0)
        .attr("x", 0 - (CONTAINER_HEIGHT / 2))
        .attr("dy", "1em")
        .style("text-anchor", "middle")
        .text(yAxisText)

    data.forEach(function (d) {
        var height = data.length > 1 ? yscale(data[0].task_id) - yscale(data[1].task_id) : CONTAINER_HEIGHT - PADDING - RANGE_END;
        var yScaleNumber = data.length > 1 ? yscale(d.task_id) : RANGE_END
        var group = svg.append('g')
            .attr('id', sanitizeId(d.task_id))
        var readTime = getBoxWidthFromArray(d, "read", xscale)
        var computeTime = getBoxWidth(d, "compute", xscale)
        var writeTime = getBoxWidthFromArray(d, "write", xscale)
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

        /* READ */
        if (partitionIO) {
            for (var i = 0; i < d.read.length; i++) {
                const r = d.read[i]
                addBox(group, xscale(r.start), yScaleNumber, height, getBoxWidth(r, "", xscale), read_color, r.id, 'read')
                if (i < d.read.length - 1) {
                    addLine(group, xscale(r.end), yScaleNumber, xscale(r.end), yScaleNumber + height)
                }
            }
        } else {
            addBox(group, readTime.start, yScaleNumber, height, readTime.end, read_color, 'Read Input', 'read')
        }
        
        /* COMPUTE */
        if (ft_point != "read" || ft_point == "none") {
            addBox(group, xscale(d.compute.start), yScaleNumber, height, computeTime, compute_color, 'Computation', 'compute')
        }

        /* WRITE */
        if (partitionIO) {
            for (var i = 0; i < d.write.length; i++) {
                const w = d.write[i]
                addBox(group, xscale(w.start), yScaleNumber, height, getBoxWidth(w, "", xscale), write_color, w.id, 'write')
                if (i < d.write.length - 1) {
                    addLine(group, xscale(w.end), yScaleNumber, xscale(w.end), yScaleNumber + height)
                }
            }
        } else {
            if ((ft_point != "read" && ft_point != "compute") || ft_point == "none") {
                addBox(group, writeTime.start, yScaleNumber, height, writeTime.end, write_color, 'Write Output', 'write')
            }
        }
        
        var tooltip = document.getElementById("tooltip-container")
        var tooltip_task_id = d3.select("#tooltip-task-id")
        var tooltip_host = d3.select("#tooltip-host")
        var tooltip_task_operation = d3.select("#tooltip-task-operation")
        var tooltip_task_operation_duration = d3.select("#tooltip-task-operation-duration")
        group.selectAll('rect')
            .on('mouseover', function () {
                tooltip.style.display = 'inline'

                d3.select(this)
                    .attr('stroke', 'gray')
                    .style('stroke-width', '1')
            })
            .on('mousemove', function () {
                var offset = getOffset(document.getElementById(containerId), d3.mouse(this))
                var x = window.scrollX + offset.left + 20
                var y = window.scrollY + offset.top - 30 // 20 seems to account for the padding of the chart-block

                tooltip.style.left = x + 'px'
                tooltip.style.top = y + 'px'

                tooltip_task_id.text('TaskID: ' + d.task_id)

                tooltip_host.text('Host Name: ' + d[executionHostKey].hostname)

                const operation = d3.select(this).attr('operation')
                tooltip_task_operation.text(operation)

                const parent_group = d3.select(this).attr('parentGroup')
                var durationFull = findDuration(data, d.task_id, parent_group)
                if (parent_group != "failed" && parent_group != "terminated") {
                    var duration = toFiveDecimalPlaces(durationFull)
                    tooltip_task_operation_duration.text('Duration: ' + duration + 's')
                } else {
                    tooltip_task_operation_duration.text('')
                }

            })
            .on('mouseout', function () {
                tooltip.style.display = 'none'

                d3.select(this)
                    .attr('stroke', 'none')
            })
    })
}
