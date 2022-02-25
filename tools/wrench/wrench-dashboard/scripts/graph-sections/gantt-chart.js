let ganttChart = null;

const ganttChartScales = {
    yAxes: [{
        stacked: true,
        ticks: {
            reverse: true
        },
        scaleLabel: {
            display: true,
            labelString: 'Tasks ID'
        }
    }],
    xAxes: [{
        scaleLabel: {
            display: true,
            labelString: 'Time (seconds)'
        }
    }]
};

function cleanGanttChart() {
    if (ganttChart !== null) {
        ganttChart.destroy();
    }
}

function processIO(taskIO) {
    let minStart = 0;
    let maxEnd = 0;

    if (taskIO && Object.keys(taskIO).length > 0) {
        minStart = Number.MAX_VALUE;
        let ioKeys = Object.keys(taskIO);
        ioKeys.forEach(function (ioKey) {
            let tIO = taskIO[ioKey];
            minStart = Math.min(tIO.start, minStart);
            maxEnd = Math.max(tIO.end, maxEnd);
        });
    }
    return [minStart, maxEnd];
}

/**
 * Generates the gantt chart.
 *
 * @param rawData: simulation data
 * @param containedId: id for the chart container element
 * @param zoom: whether to allow zoom functionality in the chart
 * @param label: labels to be displayed
 */
function generateGanttChart(rawData, containedId = null, zoom = true, label = null) {
    cleanGanttChart();
    const containerId = containedId ? containedId : "graph-container";
    let ctx = document.getElementById(containerId);

    let labels = label ? label : {
        read: {display: true, label: "Reading Input"},
        compute: {display: true, label: "Performing Computation"},
        write: {display: true, label: "Writing Output"},
    };

    const colors = {
        "read": '#44AA99',
        "compute": '#DDCC77',
        "write": '#88CCEE'
    };

    // prepare data
    let data = {
        labels: [],
        datasets: [
            {
                data: [],
                backgroundColor: [],
                host: [],
                label: labels.read.label
            },
            {
                data: [],
                backgroundColor: [],
                host: [],
                label: labels.compute.label
            },
            {
                data: [],
                host: [],
                backgroundColor: [],
                label: labels.write.label
            }
        ]
    }
    let zoomMaxRange = 0;

    let keys = Object.keys(rawData.tasks);
    keys.forEach(function (key) {
        let task = rawData.tasks[key];
        data.labels.push(task.task_id);

        // read
        data.datasets[0].data.push(processIO(task.read));
        data.datasets[0].backgroundColor.push(colors.read);
        data.datasets[0].host.push(task.execution_host.hostname);

        // compute
        data.datasets[1].data.push([task.compute.start, task.compute.end]);
        data.datasets[1].backgroundColor.push(colors.compute);
        data.datasets[1].host.push(task.execution_host.hostname);

        // write
        data.datasets[2].data.push(processIO(task.write));
        data.datasets[2].backgroundColor.push(colors.write);
        data.datasets[2].host.push(task.execution_host.hostname);

        zoomMaxRange = Math.max(zoomMaxRange, task.whole_task.end);
    });

    // parse labels
    let datasets = []
    if (labels.read.display) {
        datasets.push(data.datasets[0]);
    }
    if (labels.compute.display) {
        datasets.push(data.datasets[1]);
    }
    if (labels.write.display) {
        datasets.push(data.datasets[2]);
    }

    data.datasets = datasets;

    // zoom properties
    let pluginsProperties = definePluginsProperties(zoom, zoomMaxRange);

    ganttChart = new Chart(ctx, {
        type: 'horizontalBar',
        data: data,
        options: {
            scales: ganttChartScales,
            tooltips: {
                position: 'nearest',
                mode: 'point',
                intersect: 'false',
                callbacks: {
                    label: function (tooltipItem, data) {
                        let value = tooltipItem.value.replace("[", "").replace("]", "").split(", ");
                        let runtime = value[1] - value[0];
                        if (runtime > 0) {
                            let label = data.datasets[tooltipItem.datasetIndex].label || '';
                            if (label) {
                                label += ': ' + runtime.toFixed(3) + "s";
                            }
                            return label;
                        }
                        return "";
                    },
                    afterBody: function (tooltipItem, data) {
                        return "Execution Host: " + data.datasets[tooltipItem[0].datasetIndex].host[tooltipItem[0].index];
                    }
                }
            },
            plugins: pluginsProperties
        }
    });
}

function generateHostGanttChart(rawData, zoom = true) {
    cleanGanttChart();
    const containerId = "graph-container";
    let ctx = document.getElementById(containerId);

    // prepare data
    let data = {
        labels: [],
        datasets: []
    }
    let zoomMaxRange = 0;

    // define host colors
    let hosts = {}
    let hostIndex = 0;
    let keys = Object.keys(rawData.tasks);
    keys.forEach(function (key) {
        let task = rawData.tasks[key];
        if (!(task.execution_host.hostname in hosts)) {
            hosts[task.execution_host.hostname] = hostIndex++;
            data.datasets.push({
                data: [],
                details: [],
                backgroundColor: getRandomColor(),
                label: ''
            });
        }
    });

    keys.forEach(function (key) {
        let task = rawData.tasks[key];
        data.labels.push(task.task_id);

        let index = hosts[task.execution_host.hostname];
        for (let i = 0; i < data.datasets.length; i++) {
            if (i !== index) {
                data.datasets[i].data.push([0, 0]);
                data.datasets[i].details.push({});
            } else {
                let read = processIO(task.read);
                let write = processIO(task.write);
                let runtime = [0, 0];
                if (read[1] - read [0] !== 0) {
                    runtime[0] = read[0];
                } else if (task.compute.end - task.compute.start !== 0) {
                    runtime[0] = task.compute.start;
                } else {
                    runtime[0] = write[0];
                }
                if (write[1] !== 0) {
                    runtime[1] = write[1];
                } else if (task.compute.end !== 0) {
                    runtime[1] = task.compute.end;
                } else {
                    runtime[1] = read[1];
                }
                data.datasets[index].data.push(runtime);
                data.datasets[index].details.push({
                    read: read[1] - read[0],
                    write: write[1] - write[0],
                    compute: task.compute.end - task.compute.start
                });
            }
            data.datasets[index].label = task.execution_host.hostname;
        }
        zoomMaxRange = Math.max(zoomMaxRange, task.whole_task.end);
    });

    // zoom properties
    let pluginsProperties = definePluginsProperties(zoom, zoomMaxRange);

    ganttChart = new Chart(ctx, {
        type: 'horizontalBar',
        data: data,
        options: {
            scales: ganttChartScales,
            tooltips: {
                position: 'nearest',
                mode: 'point',
                intersect: 'false',
                callbacks: {
                    label: function (tooltipItem, data) {
                        return "";
                    },
                    afterBody: function (tooltipItem, data) {
                        let keys = Object.keys(tooltipItem);
                        let datasetIndex = 0;
                        let details = {};
                        keys.forEach(function (key) {
                            let item = tooltipItem[key];
                            if (item.value !== "[0, 0]") {
                                datasetIndex = key;
                                details = data.datasets[key].details[item.index];
                            }
                        });
                        let content = "";
                        if (details.read > 0) {
                            content += "Reading Input: " + details.read.toFixed(3) + "s\n";
                        }
                        if (details.compute > 0) {
                            content += "Performing Computation: " + details.compute.toFixed(3) + "s\n";
                        }
                        if (details.write > 0) {
                            content += "Writing Output: " + details.write.toFixed(3) + "s\n";
                        }
                        content += "Execution Host: " + data.datasets[datasetIndex].label;
                        return content;
                    }
                }
            },
            plugins: pluginsProperties
        }
    });
}
