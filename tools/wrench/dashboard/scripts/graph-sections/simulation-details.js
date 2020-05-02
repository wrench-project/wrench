/*
    data: simulation data array
    tableId: id of the <table> element
    tableBodyId: id of the <tbody> element
    tdClass: class name you want applied to each td
*/
function populateWorkflowTaskDataTable(data, tableId, tableBodyId, tdClass) {

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
        let read_duration = getDuration(read_start, read_end);

        let compute_start = convertToTableFormat(task, "compute", "start");
        let compute_end = convertToTableFormat(task, "compute", "end");
        let compute_duration = getDuration(compute_start, compute_end);

        let write_start = convertToTableFormat(task, "write", "start");
        let write_end = convertToTableFormat(task, "write", "end");
        let write_duration = getDuration(write_start, write_end);

        let task_duration = getDuration(read_start, write_end);

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
