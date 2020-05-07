const workflowSummaryInnerHtml = `
    <colgroup>
        <col span="1" class='overall-metrics-table-col read-col'></col>
        <col span="1" class='overall-metrics-table-col write-col'></col>
    </colgroup>
    <thead>
        <tr>
            <th class='task-details-table-td'>Metric</th>
            <th class='task-details-table-td'>Value</th>
        </tr>
    </thead>`

const workflowSummaryHtml =  `
    <table id='overall-metrics-table'>
        ${workflowSummaryInnerHtml}
    </table>
`