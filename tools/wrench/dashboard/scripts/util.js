const getDuration = (start, end) => {
    if (start === "Failed" || start === "Terminated") {
        return start
    } else if (end === "Failed" || end === "Terminated") {
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
            if (section == "read" || section == "write") {
                if (Object.keys(currData[section]).length > 0) {
                    var duration = 0
                    for (key in Object.keys(currData[section])) {
                        duration += currData[section][key].end - currData[section][key].start
                    }
                    return duration
                }
                return 0
            } else {
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
}

function convertToTableFormat(d, section, property) {
    let metric = 0;
    if (section === "read" || section === "write") {
        metric = property === "start" ? Number.MAX_VALUE : 0;
        for (i in d[section]) {
            metric = property === "start" ? d[section][i][property] < metric ?
                d[section][i][property] : metric :
                d[section][i][property] > metric ? d[section][i][property] : metric;
        }
    } else {
        metric = d[section][property];
        if (metric === -1) {
            if (d.failed !== -1) {
                return "Failed";
            }
            if (d.terminated !== -1) {
                return "Terminated";
            }
        }
    }
    return toFiveDecimalPlaces(metric);
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
            .attr("id", "workflow-execution-chart-legend-failed")
            .text("Failed During Execution")
        legend.append("small")
            .attr("class", "inline-block")
            .attr("id", "workflow-execution-chart-legend-terminated")
            .text("Terminated By User")
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
    let taskOverlap = {};
    data.forEach(function (d) {
        let taskStart = d.whole_task.start;
        let taskEnd = determineTaskEnd(d);

        if (d.execution_host.hostname in taskOverlap) {
            let i = 0;
            let placed = false;
            let executionHost = taskOverlap[d.execution_host.hostname];

            while (!placed) {
                if (executionHost[i] === undefined) {
                    executionHost[i] = [];
                }
                let overlap = false
                for (let j = 0; j < executionHost[i].length; j++) {
                    let t = executionHost[i][j];
                    let currTaskStart = t.whole_task.start;
                    let currTaskEnd = determineTaskEnd(t);
                    if (taskEnd >= currTaskStart && taskStart <= currTaskEnd) {
                        i++;
                        overlap = true;
                        break;
                    }
                }
                if (!overlap) {
                    executionHost[i].push(d);
                    placed = true;
                }
            }
        } else {
            taskOverlap[d.execution_host.hostname] = [[d]];
        }
    })
    return taskOverlap
}

function searchOverlap(taskId, taskOverlap) {
    for (let host in taskOverlap) {
        for (let key in taskOverlap[host]) {
            var currOverlap = taskOverlap[host][key]
            for (let i = 0; i < currOverlap.length; i++) {
                if (currOverlap[i].task_id === taskId) {
                    return key;
                }
            }
        }
    }
}

function extractFileContent(file) {
    return new Promise(function (resolve, reject) {
        let reader = new FileReader()
        reader.onload = function (event) {
            resolve(event.target.result);
            // document.getElementById('fileContent').textContent = event.target.result;
        }
        reader.readAsText(file);
        setTimeout(function () {
            reject()
        }, 5000)
    })
}

function processFile(files, fileType) {
    if (files.length === 0) {
        return
    }
    extractFileContent(files[0])
        .then(function (rawDataString) {
            switch (fileType) {
                case "taskData":
                    const rawData = JSON.parse(rawDataString)
                    if (!rawData.workflow_execution || !rawData.workflow_execution.tasks) {
                        break
                    }
                    data = {
                        file: files[0].name,
                        contents: rawData.workflow_execution.tasks
                    }
                    break
                case "energy":
                    energyData = JSON.parse(rawData)
                    break
            }
            initialise()
        })
        .catch(function () {
            return
        })
}

function sanitizeId(id) {
    id = id.replace('#', '')
    id = id.replace(' ', '')
    return id
}
