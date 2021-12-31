/**
 *
 * @param data
 * @param tableID
 * @param label
 */
function populateWorkflowTaskDataTable(data, tableID = null, label = null) {

    let tableId = tableID ? tableID : "task-details-table";
    const tableBodyId = tableId + "-body";
    const tdClass = "task-details-td";

    let labels = label ? label : {
        read: {display: true, label: "Reading Input"},
        compute: {display: true, label: "Performing Computation"},
        write: {display: true, label: "Writing Output"},
    };

    let tableContents = `
        <table class="task-details-table" id='${tableId}'>
            <colgroup>
                <col span="1"></col>`;

    if (labels.read.display) {
        tableContents += `<col span="3" class="read-col"></col>`;
    }
    if (labels.compute.display) {
        tableContents += `<col span="3" class="compute-col"></col>`;
    }
    if (labels.write.display) {
        tableContents += `<col span="3" class="write-col"></col>`;
    }

    tableContents += `
                <col span="1"></col>
            </colgroup>
            <thead class="${tableId}">
                <tr>
                    <td></td>`;

    if (labels.read.display) {
        tableContents += `<td colspan="3" class="text-center ${tdClass}" style="color: #44AA99 !important">` + labels.read.label + `</td>`;
    }
    if (labels.compute.display) {
        tableContents += `<td colspan="3" class="text-center ${tdClass}" style="color: #E69F00 !important">` + labels.compute.label + `</td>`;
    }
    if (labels.write.display) {
        tableContents += `<td colspan="3" class="text-center ${tdClass}" style="color: #0072B2 !important">` + labels.write.label + `</td>`;
    }

    tableContents += `                  
                    <td></td>
                </tr>
                <tr>
                    <th scope="col" class="task-details-table-header">TaskID</th>`;

    if (labels.read.display) {
        tableContents += `
            <th scope="col" class="task-details-table-header">Start Time</th>
            <th scope="col" class="task-details-table-header">End Time</th>
            <th scope="col" class="task-details-table-header">Duration</th>`;
    }
    if (labels.compute.display) {
        tableContents += `
            <th scope="col" class="task-details-table-header">Start Time</th>
            <th scope="col" class="task-details-table-header">End Time</th>
            <th scope="col" class="task-details-table-header">Duration</th>`;
    }
    if (labels.write.display) {
        tableContents += `
            <th scope="col" class="task-details-table-header">Start Time</th>
            <th scope="col" class="task-details-table-header">End Time</th>
            <th scope="col" class="task-details-table-header">Duration</th>`;
    }
    tableContents += `        
                    <th scope="col" class="task-details-table-header">Task Duration</th>
                </tr>
            </thead>
    
            <tbody class="task-details-table" id="${tableBodyId}">
            </tbody>
        </table >`;

    document.getElementById(tableId).innerHTML = tableContents;

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

        if (labels.read.display) {
            tr.append("td")
                .html(read_start)
                .attr("class", tdClass)
                .attr("style", 'color: #44AA99 !important');
            tr.append("td")
                .html(read_end)
                .attr("class", tdClass)
                .attr("style", 'color: #44AA99 !important');
            tr.append("td")
                .html(read_duration)
                .attr("class", tdClass)
                .attr("style", 'color: #44AA99 !important');
        }

        if (labels.compute.display) {
            tr.append("td")
                .html(compute_start)
                .attr("class", tdClass)
                .attr("style", 'color: #E69F00 !important');
            tr.append("td")
                .html(compute_end)
                .attr("class", tdClass)
                .attr("style", 'color: #E69F00 !important');
            tr.append("td")
                .html(compute_duration)
                .attr("class", tdClass)
                .attr("style", 'color: #E69F00 !important');
        }

        if (labels.write.display) {
            tr.append("td")
                .html(write_start)
                .attr("class", tdClass)
                .attr("style", 'color: #0072B2 !important');
            tr.append("td")
                .html(write_end)
                .attr("class", tdClass)
                .attr("style", 'color: #0072B2 !important');
            tr.append("td")
                .html(write_duration)
                .attr("class", tdClass)
                .attr("style", 'color: #0072B2 !important');
        }

        tr.append("td")
            .html(task_duration)
            .attr("class", tdClass);
    })
}
