/*
    data: task data
*/
function getOverallWorkflowMetrics(data) {
    const tableContainer = "overall-metrics-table";
    const taskClass = "task-details-table-td";
    let hosts = new Set();
    let noFailed = 0;
    let noTerminated = 0;
    let overallStartTime = data[0].whole_task.start;
    let overallEndTime = 0;
    let noTasks = data.length;
    let totalReadDuration = 0;
    let totalComputeDuration = 0;
    let totalWriteDuration = 0;
    data.forEach(function (d) {
        let currHost = d[executionHostKey];
        hosts.add(currHost);

        if (d.failed !== -1) {
            noFailed++;
        }

        if (d.terminated !== -1) {
            noTerminated++;
        }

        let whole_task = d.whole_task;
        if (whole_task.start < overallStartTime) {
            overallStartTime = whole_task.start;
        }
        if (whole_task.end > overallEndTime) {
            overallEndTime = whole_task.end;
        }

        let readDuration = getDuration(d, "read");
        totalReadDuration += readDuration;

        let computeDuration = getDuration(d, "compute");
        totalComputeDuration += computeDuration;

        let writeDuration = getDuration(d, "write");
        totalWriteDuration += writeDuration;
    })

    let averageReadDuration = totalReadDuration / noTasks;
    let averageComputeDuration = totalComputeDuration / noTasks;
    let averageWriteDuration = totalWriteDuration / noTasks;

    let totalHosts = hosts.size;
    let noSuccesful = noTasks - (noFailed + noTerminated);
    let totalDuration = overallEndTime - overallStartTime;

    let metrics = {
        totalHosts: {
            value: totalHosts,
            display: 'Total Hosts Utilized'
        },
        totalDuration: {
            value: toFiveDecimalPlaces(totalDuration),
            display: 'Total Workflow Duration',
            unit: 's'
        },
        noTasks: {
            value: noTasks,
            display: 'Number of Tasks'
        },
        noFailed: {
            value: noFailed,
            display: 'Failed Tasks'
        },
        noTerminated: {
            value: noTerminated,
            display: 'Terminated Tasks'
        },
        noSuccesful: {
            value: noSuccesful,
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

    let div = d3.select(`#${tableContainer}`);
    div.html("");

    for (let key in metrics) {
        let currMetric = metrics[key]
        let value = currMetric.value
        let display = currMetric.display
        let unit = currMetric.unit
        if (unit === undefined) {
            unit = ''
        }
        if (value > 0) {
            div.append('div')
                .html('' +
                    '<div class="content">' +
                    '  ' + display + '' +
                    '  <div class="header">' + value + unit + '</div>' +
                    '</div>')
                .attr('class', 'item summary')
                .attr('style', 'padding-left: 1em !important');
        }
    }
}
