const simulationGraphHtml = `
    <div class="ui grey tertiary menu small" style="margin-top: -0.5em">
        <div class="header item">
            Chart Options
        </div>
        <a class="item" onclick="toggleView(this)">
            <i class="exchange icon"></i>
            Switch to Host View
        </a>
        <a class="disabled item">
            <i class="columns icon"></i>
            Show I/O Partitions
        </a>
    </div>
    <canvas id="graph-container"></canvas>
`
