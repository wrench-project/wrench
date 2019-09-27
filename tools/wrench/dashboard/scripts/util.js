const getDuration = (start, end) => {
    if (start === "Failed" || start === "Terminated") {
        return start
    }
    else if (end === "Failed" || end === "Terminated") {
        return end
    } else {
        return toFiveDecimalPlaces(end - start)
    }
}

const toFiveDecimalPlaces = d3.format('.5f')

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

function convertToTableFormat(d, section, startEnd) {
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

function getRandomColour() {
    var letters = '0123456789ABCDEF';
    var colour = '#';
    for (var i = 0; i < 6; i++) {
      colour += letters[Math.floor(Math.random() * 16)];
    }
    return colour;
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