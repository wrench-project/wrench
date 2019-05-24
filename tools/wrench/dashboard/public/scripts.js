var data={}
var energyData=[{}]
var currGraphState = "taskView"
var hostColours = {}
var currentlySelectedHost = {hostName: "", id: ""}
var firstVisit

function initialise() {
    var noFileDiv = document.getElementById("no-file")
    var mainBodyDiv = document.getElementById("main-body")
    if (localStorage.getItem("firstVisit") === "null") {
        localStorage.setItem("firstVisit", true)
        firstVisit = true
    }
    if (data.file === undefined) {
        noFileDiv.style.display = "block"
        mainBodyDiv.style.display = "none"
    } else {
        noFileDiv.style.display = "none"
        mainBodyDiv.style.display = "block"
        generateGraph(data.contents, "graph-container")
        populateLegend("taskView")
        populateWorkflowTaskDataTable(data.contents)
        getOverallWorkflowMetrics(data.contents)
        generate3dGraph(data.contents)
    }
}

function generateConsumedEnergyGraph() {
    eData = [];
    xAxisMarks = ["x"];
    // Iterate through data for each host
    for(var i = 0; i < energyData.length; i++) {
        var hostData = energyData[i]["consumed_energy_trace"];

        consumedEnergyData = []

        // Iterate through each energy trace for a host
        for(var j = 0; j < hostData.length; j++) {
            var trace = hostData[j];

            if (xAxisMarks.length <= hostData.length) {
                xAxisMarks.push(trace["time"])
            }

            consumedEnergyData.push(trace["joules"])
        }

        // Add host name to the front of the data for graph labeling
        consumedEnergyData.unshift(energyData[i]["hostname"])
        eData.push(consumedEnergyData)
    }
    
    eData.push(xAxisMarks)

    bb.generate({
        bindto: "#consumedEnergyGraph",
        data: {
            x : "x",
            columns: eData,
        },
        axis: {
            x: {
                min: 0,
                padding: {
                    left: 0.5,
                    right: 0.5,
                },
                label: "Time",
                tick: {
                    count: xAxisMarks.length,
                    values: xAxisMarks,
                },
            },
            y: {
                label: "Energy Consumed (joules)",
            }
        },
        legend: {
            show: true,
            position: "right",
            contents: {
                bindto: "#consumedEnergyGraphLegend",
                template: "<span style='color:#fff;padding:10px;background-color:{=COLOR}'>{=TITLE}</span>"
            }
        }
    });
}

function generatePStateGraph() {
    pStateData = [];
    xAxisMarks = ["x"];
    // Iterate through data for each host
    for(var i = 0; i < energyData.length; i++) {
        var hostData = energyData[i]["pstate_trace"];

        consumedEnergyData = []

        // Iterate through each energy trace for a host
        for(var j = 0; j < hostData.length; j++) {
            var trace = hostData[j]

            if (xAxisMarks.length <= hostData.length) {
                xAxisMarks.push(trace["time"])
                // console.log(trace["time"])
            }

            consumedEnergyData.push(trace["pstate"])
        }

        // Add host name to the front of the data for graph labeling
        consumedEnergyData.unshift(energyData[i]["hostname"])
        pStateData.push(consumedEnergyData)
    }
    
    pStateData.push(xAxisMarks)

    bb.generate({
        bindto: "#pStateGraph",
        data: {
            x : "x",
            columns: pStateData,
        },
        axis: {
            x: {
                min: 0,
                padding: {
                    left: 0.5,
                    right: 0.5,
                },
                label: "Time",
                tick: {
                    count: xAxisMarks.length,
                    values: xAxisMarks,
                },
            },
            y: {
                label: "Power State",
            }
        },
        legend: {
            show: true,
            position: "right",
            contents: {
                bindto: "#pStateGraphLegend",
                template: "<span style='color:#fff;padding:10px;background-color:{=COLOR}'>{=TITLE}</span>"
            }
        }
    });
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

function getRandomColour() {
    var letters = '0123456789ABCDEF';
    var colour = '#';
    for (var i = 0; i < 6; i++) {
      colour += letters[Math.floor(Math.random() * 16)];
    }
    return colour;
}

function populateMetadata() {
    var modified = data.modified
    var file = data.file
    document.getElementById('updated').innerHTML = `Data from file "${file}" was last updated on ${new Date(modified)}`
}

function generateGraph(data, containerId) {
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
    const CONTAINER_WIDTH = 1000
    const CONTAINER_HEIGHT = 1000
    const PADDING = 60
    var toFiveDecimalPlaces = d3.format('.5f')
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
        .range([CONTAINER_HEIGHT - PADDING, 30])
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
        var group = svg.append('g')
           .attr('id', d.task_id)
        var readTime = getBoxWidth(d, "read", xscale)
        var computeTime = getBoxWidth(d, "compute", xscale)
        var writeTime = getBoxWidth(d, "write", xscale)
        var ft_point = determineFailedOrTerminatedPoint(d)
        if (ft_point != "none") {
            var x_ft = xscale(d.terminated == -1 ? d.failed : d.terminated)
            var height_ft = yscale(data[0].task_id)-yscale(data[1].task_id)
            var y_ft = yscale(d.task_id)
            var colour_ft = d.terminated == -1 ? 'orange' : 'red'
            var class_ft = d.terminated == -1 ? 'failed' : 'terminated'
            group.append('rect')
                .attr('x', x_ft)
                .attr('y', y_ft)
                .attr('height', height_ft)
                .attr('width', '5px')
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
        var tooltip = document.getElementById('tooltip-container')
        var tooltip_task_id                 = d3.select('#tooltip-task-id')
        var tooltip_host                    = d3.select('#tooltip-host')
        var tooltip_task_operation          = d3.select('#tooltip-task-operation')
        var tooltip_task_operation_duration = d3.select('#tooltip-task-operation-duration')
        group.selectAll('rect')
            .on('mouseover', function() {
                tooltip.style.display = 'inline'

                d3.select(this)
                    .attr('stroke', 'gray')
                    .style('stroke-width', '1')
            })
            .on('mousemove', function() {
                var offset = getOffset(document.getElementById("graph-container"), d3.mouse(this))
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

function convertToTableFormat(d, section, startEnd) {
    var toFiveDecimalPlaces = d3.format('.5f')
    var metric = d[section][startEnd]
    if (metric === -1) {
        if (d.failed !== -1) {
            return "Failed"
        }
        if (d.terminated !== -1) {
            return "Terminated"
        }
    }
    return toFiveDecimalPlaces(metric)
}

function getDuration(start, end) {
    var toFiveDecimalPlaces = d3.format('.5f')
    if (start === "Failed" || start === "Terminated") {
        return start
    }
    else if (end === "Failed" || end === "Terminated") {
        return end
    } else {
        return toFiveDecimalPlaces(end - start)
    }
}

function populateWorkflowTaskDataTable(data) {
    d3.select("#task-details-table")
        .style('display', 'block')

    let task_details_table_body = d3.select("#task-details-table-body")

    const TASK_DATA = Object.assign([], data).sort(function(lhs, rhs) {
        return parseInt(lhs.task_id.slice(4)) - parseInt(rhs.task_id.slice(4))
    })
    TASK_DATA.forEach(function(task) {
        var task_id = task['task_id']

        var read_start       = convertToTableFormat(task, "read", "start")
        var read_end         = convertToTableFormat(task, "read", "end")
        var read_duration    = getDuration(read_start, read_end)

        var compute_start    = convertToTableFormat(task, "compute", "start")
        var compute_end      = convertToTableFormat(task, "compute", "end")
        var compute_duration = getDuration(compute_start, compute_end)

        var write_start      = convertToTableFormat(task, "write", "start")
        var write_end        = convertToTableFormat(task, "write", "end")
        var write_duration   = getDuration(write_start, write_end)

        var task_duration    = getDuration(write_end, read_start)
        if (Number.isNaN(task_duration)) {
            task_duration = Math.abs(task_duration)
        }

        var tr = task_details_table_body
            .append("tr")
            .attr("id", task_id)

        tr.append("td")
            .html(task_id)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(read_start)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(read_end)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(read_duration)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(compute_start)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(compute_end)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(compute_duration)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(write_start)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(write_end)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(write_duration)
            .attr("class", "task-details-table-td")
        tr.append("td")
            .html(task_duration)
            .attr("class", "task-details-table-td")
    })
}

function getOverallWorkflowMetrics(data) {
    var toFiveDecimalPlaces = d3.format('.5f')

    var hosts = new Set()
    var noFailed = 0
    var noTerminated = 0
    var overallStartTime = data[0].whole_task.start
    var overallEndTime = 0
    var noTasks = data.length
    var averageReadDuration
    var averageComputeDuration
    var averageWriteDuration

    data.forEach(function(d) {
        var currHost = d.execution_host
        hosts.add(currHost)

        if (d.failed != -1) {
            noFailed++
        }

        if (d.terminated != -1) {
            noTerminated++
        }
    
        var whole_task = d.whole_task
        if (whole_task.start < overallStartTime) {
            overallStartTime = whole_task.start
        }
        if (whole_task.end > overallEndTime) {
            overallEndTime = whole_task.end
        }

        var read = d.read
        var compute = d.compute
        var write = d.write

        var totalReadDuration = 0
        var totalComputeDuration = 0
        var totalWriteDuration = 0

        var readDuration = getDuration(read.start, read.end)
        if (readDuration !== read.start && readDuration !== read.end) {
            totalReadDuration += readDuration
        }

        var computeDuration = getDuration(compute.start, compute.end)
        if (computeDuration !== compute.start && computeDuration !== compute.end) {
            totalComputeDuration += computeDuration
        }

        var writeDuration = getDuration(write.start, write.end)
        if (writeDuration !== write.start && writeDuration !== write.end) {
            totalWriteDuration == writeDuration
        }

        averageReadDuration = totalReadDuration / noTasks
        averageComputeDuration = totalComputeDuration / noTasks
        averageWriteDuration = totalWriteDuration / noTasks
    })

    var totalHosts = hosts.size
    var noSuccesful = noTasks - (noFailed + noTerminated)
    var totalDuration = overallEndTime - overallStartTime

    var metrics = {
        totalHosts: {
            value: toFiveDecimalPlaces(totalHosts),
            display: 'Total Hosts Utilized'
        },
        totalDuration: {
            value: toFiveDecimalPlaces(totalDuration),
            display: 'Total Workflow Duration',
            unit: 's'
        },
        noTasks: {
            value: toFiveDecimalPlaces(noTasks),
            display: 'Number of Tasks'
        },
        noFailed: {
            value: toFiveDecimalPlaces(noFailed),
            display: 'Failed Tasks'
        },
        noTerminated: {
            value: toFiveDecimalPlaces(noTerminated),
            display: 'Terminated Tasks'
        },
        noSuccesful: {
            value: toFiveDecimalPlaces(noSuccesful),
            display: 'Succesful Tasks'
        },
        averageReadDuration: {
            value: toFiveDecimalPlaces(averageReadDuration),
            display: 'Average Read Duration',
            unit: 's'
        },
        averageComputeDuration: {
            value: toFiveDecimalPlaces(averageComputeDuration),
            display: 'Average Compute Duration',
            unit: 's'
        },
        averageWriteDuration: {
            value: toFiveDecimalPlaces(averageWriteDuration),
            display: 'Average Write Duration',
            unit: 's'
        }
    }

    for (var key in metrics) {
        var table = d3.select('#overall-metrics-table')
        var tr = table.append('tr')

        var currMetric = metrics[key]
        var value = currMetric.value
        var display = currMetric.display
        var unit = currMetric.unit
        if (unit === undefined) {
            unit = ''
        }

        tr.append('td')
            .html(display)
            .attr('class', 'task-details-table-td')
        tr.append('td')
            .html(`${value} ${unit}`)
            .attr('class', 'task-details-table-td')
    }
}

function showHideArrow(id, arrowId) {
    var overallDiv = $(`#${id}`)
    var arrow = $(`#${arrowId}`)
    overallDiv.slideToggle()
    if (overallDiv.hasClass('hidden')) {
        arrow.rotate({animateTo: 180})
        overallDiv.removeClass('hidden')
    } else {
        arrow.rotate({animateTo: 0})
        overallDiv.addClass('hidden')
    }
}

function getHostNames(data) {
    var hostNames = new Set();

    data.forEach(function(task) {
        var hostName = task.execution_host.hostname
        hostNames.add(hostName)
    })

    hostNames = Array.from(hostNames)
    return hostNames
}

function hostColoursJSONPopulated() {
    for (var key in hostColours) {
        return true
    }
    return false
}

function switchToHostView(data, selectedHost) {
    var hostNames = getHostNames(data)

    if (!hostColoursJSONPopulated()) {
        hostNames.forEach(function(hostName) {
            var colour = getRandomColour()
            while (colour === '#FF0000' || colour === '#FFA500') {
                colour = getRandomColour()
            }
            hostColours[hostName] = colour
        })
    }
    

    data.forEach(function(task) {
        var hostName = task.execution_host.hostname
        var taskRead = d3.select(`#${task.task_id} .read`)
        var taskCompute = d3.select(`#${task.task_id} .compute`)
        var taskWrite = d3.select(`#${task.task_id} .write`)

        taskRead.style("fill", hostColours[hostName])
        taskRead.style("opacity", 1)

        taskCompute.style("fill", hostColours[hostName])
        taskCompute.style("opacity", 1)

        taskWrite.style("fill", hostColours[hostName])
        taskWrite.style("opacity", 1)

        if (selectedHost !== '' && selectedHost !== hostName) {
            taskRead.style("fill", "gray")
            taskRead.style("opacity", 0.2)

            taskCompute.style("fill", "gray")
            taskCompute.style("opacity", 0.2)

            taskWrite.style("fill", "gray")
            taskWrite.style("opacity", 0.2)
        }
    })
}

function legendHover(hostName, id, alreadySelected) {
    if (currentlySelectedHost.hostName !== '') {
        return
    }
    var legendElement = d3.select(`#${id}`)
    if (alreadySelected) {
        legendElement.style("font-weight", "normal")
    } else {
        legendElement.style("font-weight", "bold")
    }
    switchToHostView(data.contents, hostName)
}

function legendClick(hostName, id) {
    var legendElement = d3.select(`#${id}`)
    if (hostName === currentlySelectedHost.hostName) {
        // deselect same host
        currentlySelectedHost.hostName = ""
        currentlySelectedHost.id = ""
        legendElement.style("font-weight", "normal")
        switchToHostView(data.contents, "")
        return
    } 
    if (currentlySelectedHost.hostName !== "" && hostName !== currentlySelectedHost.hostName) {
        // deselect different host
        var currentlySelectedLegendElement = d3.select(`#${currentlySelectedHost.id}`)
        currentlySelectedLegendElement.style("font-weight", "normal")
    }
    currentlySelectedHost.hostName = hostName
    currentlySelectedHost.id = id
    legendElement.style("font-weight", "bold")
    switchToHostView(data.contents, hostName)
}

function populateLegend(currView) {
    if (currView === "taskView") {
        document.getElementById("workflow-execution-chart-legend").innerHTML = `
        <small>Legend:</small> 
        <small class="inline-block" id="workflow-execution-chart-legend-read-input">Reading Input</small>
        <small class="inline-block" id="workflow-execution-chart-legend-computation">Performing Computation</small>
        <small class="inline-block" id="workflow-execution-chart-legend-write-output">Writing Output</small>
        <small class="inline-block" id="workflow-execution-chart-legend-failed">Failed During Execution</small>
        <small class="inline-block" id="workflow-execution-chart-legend-terminated">Terminated by User</small>`
    } else if (currView === "hostView") {
        var i = 0
        document.getElementById("workflow-execution-chart-legend").innerHTML = ``
        var legend = d3.select("#workflow-execution-chart-legend")
        legend.append("small")
            .text("Legend:")
        for (var hostName in hostColours) {
            if (hostColours.hasOwnProperty(hostName)) {
                legend.append("small")
                    .attr("id", `workflow-execution-chart-legend-${i}`)
                    .attr("class", "inline-block")
                    .attr("onclick", `legendClick('${hostName}', 'workflow-execution-chart-legend-${i}')`)
                    .attr("onmouseover", `legendHover('${hostName}', 'workflow-execution-chart-legend-${i}', false)`)
                    .attr("onmouseout", `legendHover('', 'workflow-execution-chart-legend-${i}', true)`)
                    .style("border-left", `15px solid ${hostColours[hostName]}`)
                    .text(hostName)
                i++
            }
        }
        legend.append("small")
            .attr("class", "inline-block")
            .attr("id","workflow-execution-chart-legend-failed")
            .text("Failed During Execution")
        legend.append("small")
            .attr("class", "inline-block")
            .attr("id","workflow-execution-chart-legend-terminated")
            .text("Terminated By User")
    }
}

function showHostInstructions() {
    var hostInstructions = document.getElementById("host-instructions")
    var informationImg = document.getElementById("information-img")
    hostInstructions.style.display = "block"
    informationImg.style.display = "none"
}

function hideHostInstructions() {
    var hostInstructions = document.getElementById("host-instructions")
    var informationImg = document.getElementById("information-img")
    hostInstructions.style.display = "none"
    informationImg.style.display = "inline-block"
}

function toggleView() {
    var hostInstructions = document.getElementById("host-instructions")
    var informationImg = document.getElementById("information-img")
    if (currGraphState === "taskView") {
        switchToHostView(data.contents, '')
        d3.select("#y-axis-label").text("Host Name")
        populateLegend("hostView")
        d3.select("#toggle-view-button").text("Switch to Task View")
        if (firstVisit) {
            hostInstructions.style.display = "block"
            informationImg.style.display = "none"
        } else {
            hostInstructions.style.display = "none"
            informationImg.style.display = "inline-block"
        }
        currGraphState = "hostView"
    } else if (currGraphState === "hostView") {
        generateGraph(data.contents, "graph-container")
        d3.select("#y-axis-label").text("TaskID")
        populateLegend("taskView")
        d3.select("#toggle-view-button").text("Switch to Host View")
        hostInstructions.style.display = "none"
        informationImg.style.display = "none"
        currGraphState = "taskView"
    }
}

function determineTaskEnd(d) {
    var taskEnd
    if (d.terminated !== -1) {
        taskEnd = d.terminated
    } else if (d.failed !== -1) {
        taskEnd = d.failed
    } else {
        taskEnd = d.whole_task.end
    }
    return taskEnd
}

function determineTaskOverlap(data) {
    var taskOverlap = {}
    data.forEach(function(d) {
        var taskStart = d.whole_task.start
        var taskEnd = determineTaskEnd(d)
        if (Object.keys(taskOverlap).length === 0) {
            taskOverlap[0] = []
            taskOverlap[0].push(d)
        } else {
            var i = 0
            var placed = false
            while (!placed) {
                if (taskOverlap[i] === undefined) {
                    taskOverlap[i] = []
                }
                var overlap = false
                for (var j = 0; j < taskOverlap[i].length; j++) {
                    var t = taskOverlap[i][j]
                    var currTaskStart = t.whole_task.start
                    var currTaskEnd = determineTaskEnd(t)
                    if ((taskStart >= currTaskStart && taskStart <= currTaskEnd) || (taskEnd >= currTaskStart && taskEnd <= currTaskEnd)) {
                        i++
                        overlap = true
                        break
                    }
                }
                if (!overlap) {
                    taskOverlap[i].push(d)
                    placed = true
                }
            }
        }
    })
    return taskOverlap
}

function determineMaxNumCoresAllocated(data) {
    var max = 0
    data.forEach(function(d) {
        if (d.num_cores_allocated >= max) {
            max = d.num_cores_allocated
        }
    })
    return max
}

var origin = [0, 400]
startAngle = Math.PI/4
var scale = 20
var key = function(d) { return d.task_id; }

function searchOverlap(taskId, taskOverlap) {
    for (var key in taskOverlap) {
        if (taskOverlap.hasOwnProperty(key)) {
            var currOverlap = taskOverlap[key]
            for (var i = 0; i < currOverlap.length; i++) {
                if (currOverlap[i].task_id === taskId) {
                    return key
                }
            }
        }
    }
}

function makeCube(h, x, z){
    return [
        {x: x - 1, y: h, z: z + 1}, // FRONT TOP LEFT
        {x: x - 1, y: 0, z: z + 1}, // FRONT BOTTOM LEFT
        {x: x + 1, y: 0, z: z + 1}, // FRONT BOTTOM RIGHT
        {x: x + 1, y: h, z: z + 1}, // FRONT TOP RIGHT
        {x: x - 1, y: h, z: z - 1}, // BACK  TOP LEFT
        {x: x - 1, y: 0, z: z - 1}, // BACK  BOTTOM LEFT
        {x: x + 1, y: 0, z: z - 1}, // BACK  BOTTOM RIGHT
        {x: x + 1, y: h, z: z - 1}, // BACK  TOP RIGHT
    ];
}

function processData(data, tt){

    var grid3d = d3._3d()
        .shape('GRID', 20)
        .origin(origin)
        .rotateY( startAngle)
        .rotateX(-startAngle)
        .scale(scale);

    var yScale3d = d3._3d()
        .shape('LINE_STRIP')
        .origin(origin)
        .rotateY( startAngle)
        .rotateX(-startAngle)
        .scale(scale);

    var cubes3d = d3._3d()
        .shape('CUBE')
        .x(function(d){ return d.x; })
        .y(function(d){ return d.y; })
        .z(function(d){ return d.z; })
        .rotateY( startAngle)
        .rotateX(-startAngle)
        .origin(origin)
        .scale(scale)

    var svg    = d3.select('#three-d-graph-svg').append('g')
    var cubesGroup = svg.append('g').attr('class', 'cubes')
    var color  = d3.scaleOrdinal(d3.schemeCategory20)

    /* ----------- GRID ----------- */

    var xGrid = svg.selectAll('path.grid').data(data[0], key);

    xGrid
        .enter()
        .append('path')
        .attr('class', '_3d grid')
        .merge(xGrid)
        .attr('stroke', 'black')
        .attr('stroke-width', 0.3)
        .attr('fill', function(d){ return d.ccw ? 'lightgrey' : '#717171'; })
        .attr('fill-opacity', 0.9)
        .attr('d', grid3d.draw);

    xGrid.exit().remove();

     /* ----------- y-Scale ----------- */

    var yScale = svg.selectAll('path.yScale').data(data[1]);

    yScale
        .enter()
        .append('path')
        .attr('class', '_3d yScale')
        .merge(yScale)
        .attr('stroke', 'black')
        .attr('stroke-width', .5)
        .attr('d', yScale3d.draw);

    yScale.exit().remove();

        /* ----------- y-Scale Text ----------- */

    var yText = svg.selectAll('text.yText').data(data[1][0]);

    yText
        .enter()
        .append('text')
        .attr('class', '_3d yText')
        .attr('dx', '.3em')
        .merge(yText)
        .each(function(d){
            d.centroid = {x: d.rotated.x, y: d.rotated.y, z: d.rotated.z};
        })
        .attr('x', function(d){ return d.projected.x; })
        .attr('y', function(d){ return d.projected.y; })
        .text(function(d){ return d[1] <= 0 ? d[1] : ''; });

    yText.exit().remove();

    /* --------- CUBES ---------*/

    var cubes = cubesGroup.selectAll('g.cube').data(data[2], function(d){ return d.id });

    var ce = cubes
        .enter()
        .append('g')
        .attr('class', 'cube')
        .attr('fill', function(d){ return color(d.id); })
        .attr('stroke', function(d){ return d3.color(color(d.id)); })
        .merge(cubes)
        .sort(cubes3d.sort);

    cubes.exit().remove();

    /* --------- FACES ---------*/

    var faces = cubes.merge(ce).selectAll('path.face').data(function(d){ return d.faces; }, function(d){ return d.face; });

    faces.enter()
        .append('path')
        .attr('class', 'face')
        .attr('fill-opacity', 0.95)
        .classed('_3d', true)
        .merge(faces)
        .transition().duration(tt)
        .attr('d', cubes3d.draw);

    faces.exit().remove();
}

function generate3dGraph(data) {
    var grid3d = d3._3d()
        .shape('GRID', 20)
        .origin(origin)
        .rotateY( startAngle)
        .rotateX(-startAngle)
        .scale(scale)

    var yScale3d = d3._3d()
        .shape('LINE_STRIP')
        .origin(origin)
        .rotateY( startAngle)
        .rotateX(-startAngle)
        .scale(scale)

    var cubes3d = d3._3d()
        .shape('CUBE')
        .x(function(d){ return d.x; })
        .y(function(d){ return d.y; })
        .z(function(d){ return d.z; })
        .rotateY( startAngle)
        .rotateX(-startAngle)
        .origin(origin)
        .scale(scale)

    var j = 10
    var maxTime = d3.max(data, function(d) {
        return Math.max(d['whole_task'].end, d['failed'], d['terminated'])
    })
    var taskOverlap = determineTaskOverlap(data)
    var maxNumCoresAllocated = determineMaxNumCoresAllocated(data)
    xGrid = [], scatter = [], yLine = []
    for(var z = 0; z < Object.keys(taskOverlap).length; z++){
        for(var x = 0; x < maxTime; x++) {
            xGrid.push([x, 1, z])
        }
    }

    d3.range(-1, maxNumCoresAllocated + 1, 1).forEach(function(d) { yLine.push([0, -d, 0]); })

    cubesData = []
    data.forEach(function(d) {
        var h = d.num_cores_allocated
        var x = d.whole_task.start
        var z = searchOverlap(d.task_id, taskOverlap)
        // console.log(h + ' ' + x + ' ' + z)
        var cube = makeCube(h, x, z)
        cube.height = h
        cubesData.push(cube)
    })

    var data = [
        grid3d(xGrid),
        yScale3d([yLine]),
        cubes3d(cubesData)
    ];
    processData(data, 1000);

}