let hostUtilizationChart = null;

function fillEmptyValues(datasets, end, labels) {
    for (let i = datasets.length; i < end; i++) {
        datasets.push({
            data: [],
            backgroundColor: [],
            taskId: [],
            borderColor: "rgba(0, 0, 0, 0.3)",
            borderWidth: 1,
            barPercentage: 1.2,
        });
    }
    for (let i = 0; i < datasets.length; i++) {
        for (let j = datasets[i].data.length; j < labels.length - 1; j++) {
            datasets[i].data.push([0, 0]);
            datasets[i].backgroundColor.push("");
            datasets[i].taskId.push("");
        }
    }
}

function ingestData(obj, start, end, color, id) {
    obj.data.push([start, end]);
    obj.backgroundColor.push(color);
    obj.taskId.push(id);
}

function findTaskScheduling(data, hosts) {
    let keys = Object.keys(hosts);
    keys.forEach(function (key) {
        let hostTasks = [];
        // obtaining sorted list of tasks by start time
        for (let i = 0; i < data.length; i++) {
            let task = data[i];
            if (task.execution_host.hostname === key) {
                let position = -1;
                for (let j = 0; j < hostTasks.length; j++) {
                    if (hostTasks[j].whole_task.start > task.whole_task.start) {
                        position = j;
                        break;
                    }
                }
                if (hostTasks.length === 0 || position === -1) {
                    hostTasks.push(task);
                } else {
                    hostTasks.splice(position, 0, task);
                }
            }
        }
        // distributing tasks into cores
        let host = hosts[key];
        for (let i = 0; i < hostTasks.length; i++) {
            let task = hostTasks[i];
            for (let j = 0; j < host.cores; j++) {
                let inserted = false;
                for (let k = 0; k < task.num_cores_allocated; k++) {
                    if (k > 0) {
                        j++;
                    }
                    let tasks = host.tasks[j];
                    if (tasks.length === 0) {
                        tasks.push(task);
                        inserted = true;
                    } else {
                        let lastTask = tasks[tasks.length - 1];
                        if (lastTask.whole_task.end <= task.whole_task.start) {
                            tasks.push(task);
                            inserted = true;
                        }
                    }
                }
                if (inserted) {
                    break;
                }
            }
        }
    });
}

/**
 * Generates the host utilization chart
 *
 * @param rawData: simulation data
 * @param containedId: id for the chart container element
 * @param hostsList: list of host names in which compute tasks will be displayed (empty list displays all hosts)
 * @param diskHostsList: list of host names in which the disk usage will be displayed (empty list displays all hosts)
 * @param zoom: whether to allow zoom functionality in the chart
 * @param operations: type of operations to be shown ('all', 'compute', 'io', 'write', 'read')
 */
function generateHostUtilizationChart(rawData,
                                      containedId = null,
                                      hostsList = [],
                                      diskHostsList = [],
                                      zoom = true,
                                      operations = "all") {
    // clean chart
    if (hostUtilizationChart !== null) {
        hostUtilizationChart.destroy();
    }

    const containerId = containedId ? containedId : "host-utilization-chart";
    let ctx = document.getElementById(containerId);

    // prepare data
    let data = {
        labels: [],
        datasets: [],
    }
    let zoomMaxRange = 0;

    // obtain list of hosts
    let hosts = {};
    let keys = Object.keys(rawData.tasks);
    keys.forEach(function (key) {
        let task = rawData.tasks[key];
        if (hostsList.length > 0 && !(hostsList.includes(task.execution_host.hostname))) {
            return;
        }
        if (!(task.execution_host.hostname in hosts)) {
            hosts[task.execution_host.hostname] = {
                cores: task.execution_host.cores,
                tasks: {}
            }
            for (let i = 0; i < task.execution_host.cores; i++) {
                hosts[task.execution_host.hostname].tasks[i] = [];
            }
        }
    });

    // obtain additional hosts without tasks
    if (rawData.disk) {
        keys = Object.keys(rawData.disk);
        keys.forEach(function (key) {
            if (diskHostsList.length > 0 && !(diskHostsList.includes(key))) {
                return;
            }
            if (!(key in hosts)) {
                hosts[key] = {
                    cores: 1,
                    tasks: {}
                }
            }
        });
    }

    findTaskScheduling(rawData.tasks, hosts);

    // populate data
    keys = Object.keys(hosts);
    keys.forEach(function (key) {
        let host = hosts[key];

        // add disk operations
        if ((diskHostsList.length === 0 || diskHostsList.includes(key)) && (rawData.disk && rawData.disk[key])) {
            let mounts = Object.keys(rawData.disk[key]);
            mounts.forEach(function (mount) {
                let diskMounts = rawData.disk[key];

                // read operations
                if (diskMounts[mount].reads) {
                    data.labels.push(key + " (mount: " + mount + " - reads)");
                    fillEmptyValues(data.datasets, diskMounts[mount].reads.length, data.labels);
                    let index = 0;
                    for (let i = 0; i < diskMounts[mount].reads.length; i++) {
                        let operation = diskMounts[mount].reads[i];
                        ingestData(data.datasets[i], operation.start, operation.end, "#44AA99", "read");
                    }
                }

                // write operations
                if (diskMounts[mount].writes) {
                    data.labels.push(key + " (mount: " + mount + " - writes)");
                    fillEmptyValues(data.datasets, diskMounts[mount].writes.length, data.labels);
                    for (let i = 0; i < diskMounts[mount].writes.length; i++) {
                        let operation = diskMounts[mount].writes[i];
                        ingestData(data.datasets[i], operation.start, operation.end, "#88CCEE", "write");
                    }
                }
            });
        }

        // add compute tasks
        if (hostsList.length === 0 || hostsList.includes(key)) {
            let tasks = Object.keys(host.tasks);
            tasks.forEach(function (core) {
                let coreTasks = host.tasks[core];
                data.labels.push(key + " (core #" + core + ")");
                // filling empty values
                fillEmptyValues(data.datasets, coreTasks.length, data.labels, percentage = 1.2);
                for (let i = 0; i < coreTasks.length; i++) {
                    let task = coreTasks[i];
                    ingestData(data.datasets[i], task.compute.start, task.compute.end, task.color || "#DDCC77", task.task_id);
                    zoomMaxRange = Math.max(zoomMaxRange, task.compute.end);
                }
            });
        }
    });

    // zoom properties
    let pluginsProperties = definePluginsProperties(zoom, zoomMaxRange);

    hostUtilizationChart = new Chart(ctx, {
        type: 'horizontalBar',
        data: data,
        options: {
            scales: {
                yAxes: [{
                    stacked: true,
                    ticks: {
                        reverse: true
                    }
                }],
                xAxes: [{
                    scaleLabel: {
                        display: true,
                        labelString: 'Time (seconds)'
                    }
                }]
            },
            chartArea: {
                backgroundColor: "#FEF8F8"
            },
            legend: {
                display: false
            },
            tooltips: {
                position: 'nearest',
                mode: 'point',
                intersect: 'false',
                callbacks: {
                    label: function (tooltipItem, data) {
                        let value = tooltipItem.value.replace("[", "").replace("]", "").split(", ");
                        let runtime = value[1] - value[0];
                        if (runtime > 0) {
                            let label = data.datasets[tooltipItem.datasetIndex].taskId[tooltipItem.index] || '';
                            if (label) {
                                label += ': ' + runtime.toFixed(3) + "s";
                            }
                            return label;
                        }
                        return "";
                    },
                }
            },
            plugins: pluginsProperties
        }
    });
}
