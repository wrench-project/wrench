/*
    data: simulation data array
*/
function populateWorkflowTaskDataTable(data) {
    const tableId = "task-details-table" 
    const tableBodyId = "task-details-table-body"
    const tdClass = "task-details-table-td"
    document.getElementById(tableId).innerHTML = simulationDetailsHtml;
    d3.select(`#${tableId}`).style('display', 'block');

    let task_details_table_body = d3.select(`#${tableBodyId}`);

    const TASK_DATA = Object.assign([], data).sort(function (lhs, rhs) {
        return parseInt(lhs.compute.start) - parseInt(rhs.compute.start);
    });

    TASK_DATA.forEach(function (task) {
        let task_id = task['task_id'];

        let read_start = convertToTableFormat(task, "read", "start");
        let read_end = convertToTableFormat(task, "read", "end");
        let read_duration = toFiveDecimalPlaces(getDuration(task, "read"));

        let compute_start = convertToTableFormat(task, "compute", "start");
        let compute_end = convertToTableFormat(task, "compute", "end");
        let compute_duration = toFiveDecimalPlaces(getDuration(task, "compute"));

        let write_start = convertToTableFormat(task, "write", "start");
        let write_end = convertToTableFormat(task, "write", "end");
        let write_duration = toFiveDecimalPlaces(getDuration(task, "write"));

        let task_duration = toFiveDecimalPlaces(getDuration(task, "whole_task"));

        if (Number.isNaN(task_duration)) {
            task_duration = Math.abs(task_duration);
        }

        let tr = task_details_table_body
            .append("tr")
            .attr("id", task_id);
        tr.append("td")
            .html(task_id)
            .attr("class", tdClass);
        tr.append("td")
            .html(read_start)
            .attr("class", tdClass);
        tr.append("td")
            .html(read_end)
            .attr("class", tdClass);
        tr.append("td")
            .html(read_duration)
            .attr("class", tdClass);
        tr.append("td")
            .html(compute_start)
            .attr("class", tdClass);
        tr.append("td")
            .html(compute_end)
            .attr("class", tdClass);
        tr.append("td")
            .html(compute_duration)
            .attr("class", tdClass);
        tr.append("td")
            .html(write_start)
            .attr("class", tdClass);
        tr.append("td")
            .html(write_end)
            .attr("class", tdClass);
        tr.append("td")
            .html(write_duration)
            .attr("class", tdClass);
        tr.append("td")
            .html(task_duration)
            .attr("class", tdClass);
    })
}
