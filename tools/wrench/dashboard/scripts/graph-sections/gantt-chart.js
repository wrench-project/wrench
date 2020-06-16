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

    if (Object.keys(taskIO).length > 0) {
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

function generateGanttChart(rawData) {
    cleanGanttChart();
    const containerId = "graph-container";
    let ctx = document.getElementById(containerId);
    const colors = {
        'read': '#cbb5dd',
        'compute': '#f7daad',
        'write': '#abdcf4'
    };

    // prepare data
    let data = {
        labels: [],
        datasets: [
            {
                data: [],
                backgroundColor: [],
                host: [],
                label: 'Reading Input'
            },
            {
                data: [],
                backgroundColor: [],
                host: [],
                label: 'Performing Computation'
            },
            {
                data: [],
                host: [],
                backgroundColor: [],
                label: 'Writing Output'
            }
        ]
    }

    let keys = Object.keys(rawData);
    keys.forEach(function (key) {
        let task = rawData[key];
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
    });

    ganttChart = new Chart(ctx, {
        type: 'horizontalBar',
        data: data,
        options: {
            scales: ganttChartScales,
            tooltips: {
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
            }
        }
    });
}

function generateHostGanttChart(rawData) {
    cleanGanttChart();
    const containerId = "graph-container";
    let ctx = document.getElementById(containerId);

    // prepare data
    let data = {
        labels: [],
        datasets: []
    }

    // define host colors
    let hosts = {}
    let hostIndex = 0;
    let keys = Object.keys(rawData);
    keys.forEach(function (key) {
        let task = rawData[key];
        if (!(task.execution_host.hostname in hosts)) {
            hosts[task.execution_host.hostname] = hostIndex++;
            data.datasets.push({
                data: [],
                details: [],
                backgroundColor: getRandomColour(),
                label: ''
            });
        }
    });

    keys.forEach(function (key) {
        let task = rawData[key];
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
    });

    ganttChart = new Chart(ctx, {
        type: 'horizontalBar',
        data: data,
        options: {
            scales: ganttChartScales,
            tooltips: {
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
            }
        }
    });
}
