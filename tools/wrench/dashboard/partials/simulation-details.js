const simulationDetailsHtml = `
    <table class="task-details-table" id='task-details-table'>
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

        <tbody class="task-details-table" id="task-details-table-body">

        </tbody>
    </table>
`