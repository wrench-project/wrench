const executionHostKey = 'execution_host'

const dataSizeUnits = {
    B: ['B', 'Bytes', 1],
    KB: ['KB', 'Kilobytes', 3],
    MB: ['MB', 'Megabytes', 6],
    GB: ['GB', 'Gigabytes', 9],
    TB: ['TB', 'Terabytes', 12],
    PB: ['PB', 'Petabytes', 15],
}

Chart.plugins.register({
    beforeDraw: function (chart, easing) {
        if (chart.config.options.chartArea && chart.config.options.chartArea.backgroundColor) {
            let ctx = chart.chart.ctx;
            ctx.fillStyle = chart.config.options.chartArea.backgroundColor;

            let chartArea = chart.chartArea;
            ctx.fillRect(chartArea.left, chartArea.top, chartArea.right - chartArea.left, chartArea.bottom - chartArea.top);
        }
    }
});

function getRandomColor() {
    return "#" + Math.floor(Math.random() * 16777215).toString(16);
}

function extractFileContent(file) {
    return new Promise(function (resolve, reject) {
        let reader = new FileReader()
        reader.onload = function (event) {
            resolve(event.target.result);
        }
        reader.readAsText(file);
        setTimeout(function () {
            reject()
        }, 5000);
    })
}

function processFile(files) {
    if (files.length === 0) {
        return;
    }
    extractFileContent(files[0])
        .then(function (rawDataString) {
            const rawData = JSON.parse(rawDataString);
            if (!rawData.workflow_execution || !rawData.workflow_execution.tasks) {
                return;
            }
            let links = rawData.link_usage ? rawData.link_usage.links : [];
            data = {
                file: files[0].name,
                tasks: rawData.workflow_execution.tasks,
                disk: rawData.disk_operations,
                network: links,
                contents: rawData.workflow_execution.tasks // TODO: remove
            };

            if (rawData.energy_consumption) {
                energyData = rawData.energy_consumption;
            }
            document.getElementById("simulation-json").value = files[0].name;
            initialize();
        })
        .catch(function (err) {
            console.error(err)
        });
}

function definePluginsProperties(zoom, zoomMaxRange) {
    if (zoom) {
        return {
            zoom: {
                pan: {
                    enabled: true,
                    mode: 'x'
                },
                zoom: {
                    enabled: true,
                    mode: 'x',
                    rangeMin: {
                        x: 0
                    },
                    rangeMax: {
                        x: Math.ceil(zoomMaxRange)
                    },
                    threshold: 20,
                    speed: 0.05
                }
            }
        }
    }
    return {};
}

function prepareData(data) {
    const nullReplacement = {
        start: 0,
        end: 0
    };
    data.forEach(function (d) {
        if (d.read === null) {
            d.read = [nullReplacement];
        }
        if (d.compute === null) {
            d.compute = nullReplacement;
        }
        if (d.write === null) {
            d.write = [nullReplacement];
        }
    })
    return data;
}

const getDuration = (d, section) => {
    if (section === "read" || section === "write") {
        let total = 0
        if (d[section] !== null) {
            d[section].forEach(t => {
                total += (t.end - t.start);
            })
        }
        return total;
    } else if (section === "compute" || section === "whole_task") {
        if (d[section].start === -1) {
            return 0;
        } else if (d[section].end === -1) {
            if (d.terminated === -1) {
                return d.failed - d[section].start;
            } else if (d.failed === -1) {
                return d.terminated - d[section].start;
            }
        } else {
            return d[section].end - d[section].start;
        }
    }
}

/* TODO: fix this function to fix workflow summary */
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
    let taskEnd
    if (d.terminated !== -1) {
        taskEnd = d.terminated
    } else if (d.failed !== -1) {
        taskEnd = d.failed
    } else {
        taskEnd = d.whole_task.end
    }
    return taskEnd
}

function determineTaskColor(d) {
    let taskColor
    if ((!d.hasOwnProperty("color")) || (d.color === "")) {
        taskColor = "#f7daad"
    } else {
        taskColor = d.color
    }
    return taskColor
}

function brighterColor(c) {
    let bytes = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(c)
    let brightness_increment = 25
    let brighter = "#"
    for (let i = 1; i <= 3; i++) {
        let intValue = Math.min(255, brightness_increment + parseInt(bytes[i], 16));
        let hexString = intValue.toString(16);
        hexString = hexString.length === 1 ? "0" + hexString : hexString
        brighter += hexString
    }
    return brighter
}

function determineTaskOverlap(data) {
    let taskOverlap = {};
    data.forEach(function (d) {
        let taskStart = d.whole_task.start;
        let taskEnd = determineTaskEnd(d);

        if (d[executionHostKey].hostname in taskOverlap) {
            let i = 0;
            let placed = false;
            let executionHost = taskOverlap[d[executionHostKey].hostname];

            while (!placed) {
                if (executionHost[i] === undefined) {
                    executionHost[i] = [];
                }
                let overlap = false
                for (let j = 0; j < executionHost[i].length; j++) {
                    let t = executionHost[i][j];
                    let currTaskStart = t.whole_task.start;
                    let currTaskEnd = determineTaskEnd(t);
                    if (taskEnd > currTaskStart && taskStart < currTaskEnd) {
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
            taskOverlap[d[executionHostKey].hostname] = [[d]];
        }
    })
    return taskOverlap
}

function determineVerticalPosition(task, taskOverlap) {
    let task_core = 0;
    let task_host = "";
    for (let host in taskOverlap) {
        for (let task_idx in taskOverlap[host]) {
            let currOverlap = taskOverlap[host][task_idx];
            for (let i = 0; i < currOverlap.length; i++) {
                if (currOverlap[i].task_id === task.task_id) {
                    task_core = parseInt(task_idx);
                    task_host = host;
                    break;
                }
            }
        }
    }

    let v_pos = 0;
    for (let host in taskOverlap) {
        if (host === task_host) {
            for (let core_num in taskOverlap[host]) {
                if (parseInt(core_num) < task_core) {
                    let curr_v_pos = 0;
                    let currOverlap = taskOverlap[host][core_num];
                    for (let i = 0; i < currOverlap.length; i++) {
                        let t = currOverlap[i];
                        if (task.whole_task.end > t.whole_task.start && task.whole_task.start < t.whole_task.end) {
                            if (curr_v_pos < t.num_cores_allocated) {
                                curr_v_pos = t.num_cores_allocated;
                            }
                        }
                    }
                    v_pos += curr_v_pos;
                }
            }
        }
    }
    return v_pos;
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


function sanitizeId(id) {
    id = id.replace(/#/g, '')
    id = id.replace(/ /g, '')
    return id
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
