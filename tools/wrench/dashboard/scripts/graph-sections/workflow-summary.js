/*
    data: task data
    tableContainer: id of the <table> which will contain the data
    taskClass: class to apply to <td> elements
*/
function getOverallWorkflowMetrics(data, tableContainer, taskClass) {
    // document.getElementById(tableContainer).innerHTML = `
    //     <colgroup>
    //         <col span="1" class='overall-metrics-table-col read-col'></col>
    //         <col span="1" class='overall-metrics-table-col write-col'></col>
    //     </colgroup>
    //     <thead>
    //         <tr>
    //             <th class='task-details-table-td'>Metric</th>
    //             <th class='task-details-table-td'>Value</th>
    //         </tr>
    //     </thead>
    // `
    var hosts = new Set()
    var noFailed = 0
    var noTerminated = 0
    var overallStartTime = data[0].whole_task.start
    var overallEndTime = 0
    var noTasks = data.length
    var averageReadDuration
    var averageComputeDuration
    var averageWriteDuration
    data.forEach(function (d) {
        var currHost = d['execution_host']
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
            totalWriteDuration += writeDuration
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
        var table = d3.select(`#${tableContainer}`)
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
            .attr('class', taskClass)
        tr.append('td')
            .html(`${value} ${unit}`)
            .attr('class', taskClass)
    }
}