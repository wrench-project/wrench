/*
    data: simulation data array
    tableId: id of the <table> element
    tableBodyId: id of the <tbody> element
    tdClass: class name you want applied to each td
*/
function populateWorkflowTaskDataTable(data, tableId, tableBodyId, tdClass) {
    document.getElementById(tableId).innerHTML = `
    <colgroup>
        <col span="1"></col>
        <col span="3" class="read-col"></col>
        <col span="3" class="compute-col"></col>
        <col span="3" class="write-col"></col>
        <col span="1"></col>
    </colgroup>
    <thead class="task-details-table">
        <tr>
            <td></td>
            <td colspan="3" class="text-center task-details-table-td">Read Input</td>
            <td colspan="3" class="text-center task-details-table-td">Computation</td>
            <td colspan="3" class="text-center task-details-table-td">Write Output</td>
            <td></td>
        </tr>
        <tr>
            <th scope="col" class="task-details-table-header">TaskID</th>
            <th scope="col" class="task-details-table-header">Start Time</th>
            <th scope="col" class="task-details-table-header">End Time</th>
            <th scope="col" class="task-details-table-header">Duration</th>
            <th scope="col" class="task-details-table-header">Start Time</th>
            <th scope="col" class="task-details-table-header">End Time</th>
            <th scope="col" class="task-details-table-header">Duration</th>
            <th scope="col" class="task-details-table-header">Start Time</th>
            <th scope="col" class="task-details-table-header">End Time</th>
            <th scope="col" class="task-details-table-header">Duration</th>
            <th scope="col" class="task-details-table-header">Task Duration</th>
        </tr>
    </thead>

    <tbody class="task-details-table" id="task-details-table-body"></tbody>
    `
    d3.select(`#${tableId}`)
        .style('display', 'block')

    let task_details_table_body = d3.select(`#${tableBodyId}`)

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
            .attr("class", tdClass)
        tr.append("td")
            .html(read_start)
            .attr("class", tdClass)
        tr.append("td")
            .html(read_end)
            .attr("class", tdClass)
        tr.append("td")
            .html(read_duration)
            .attr("class", tdClass)
        tr.append("td")
            .html(compute_start)
            .attr("class", tdClass)
        tr.append("td")
            .html(compute_end)
            .attr("class", tdClass)
        tr.append("td")
            .html(compute_duration)
            .attr("class", tdClass)
        tr.append("td")
            .html(write_start)
            .attr("class", tdClass)
        tr.append("td")
            .html(write_end)
            .attr("class", tdClass)
        tr.append("td")
            .html(write_duration)
            .attr("class", tdClass)
        tr.append("td")
            .html(task_duration)
            .attr("class", tdClass)
    })
}
