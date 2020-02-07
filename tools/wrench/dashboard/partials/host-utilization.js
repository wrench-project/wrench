const hostUtilizationHtml = `
    <div class="chart" id="host-utilization-chart">

        <div class="container legend" id="host-utilization-chart-legend">
            <small class="inline-block" id="host-utilization-chart-legend-compute-time">Compute Time</small>
            <small class="inline-block" id="host-utilization-chart-legend-idle-time">Idle Time</small>
        </div>

        <div class="text-left" id="host-utilization-chart-tooltip">
            <span id="host-utilization-chart-tooltip-task-id"></span><br>
            <span id="host-utilization-chart-tooltip-compute-time"></span><br>
        </div>

    </div>
`