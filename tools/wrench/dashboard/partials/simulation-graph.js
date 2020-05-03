const simulationGraphTooltipHtml = `
    <div class="text-left" id="tooltip-container">
        <span id="tooltip-task-id"></span><br/>
        <span id="tooltip-host"></span><br/>
        <span id="tooltip-task-operation"></span><br/>
        <span id="tooltip-task-operation-duration"></span>
    </div>
`

const simulationGraphHtml = `
    <div id='button-and-information-container'>
        <button id='toggle-view-button' onclick="toggleView()">Switch to Host View</button>
        <img id='information-img' class='information-img' src='public/img/information.png' onclick='showInstructions("host-instructions", "information-img")'/>
    </div>
    <div id='host-instructions' class='instructions-container'>
        <div id='host-instructions-close-button-container' class='instructions-close-button-container'>
            <img id='host-instructions-close-button' class='instructions-close-button' src='public/img/close-button.png' onclick='hideInstructions("host-instructions", "information-img")'/>
        </div>
        <p id='host-instructions-text'>Hover over or click on a host on the legend to isolate that host on the graph. Click on a selected host to deselect it. or just select a different host on the legend</p>
    </div>
    <div class="container legend" id="workflow-execution-chart-legend"></div>
    <div id="graph-container"></div>
`