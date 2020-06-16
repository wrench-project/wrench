// const simulationGraphTooltipHtml = `
//     <div class="text-left" id="tooltip-container">
//         <span id="tooltip-task-id"></span><br/>
//         <span id="tooltip-host"></span><br/>
//         <span id="tooltip-task-operation"></span><br/>
//         <span id="tooltip-task-operation-duration"></span>
//     </div>
// `

const simulationGraphHtml = `
    <div id='button-and-information-container'>
        <button type="button" class="btn btn-secondary btn-sm" id="toggle-view-button" onclick="toggleView(this)">
            Switch to Host View
        </button>
        <img id='information-img' class='information-img' src='public/img/information.png' onclick='showInstructions("host-instructions", "information-img")'/>
        <button id='show-io-view-button' onclick="showIoView()" style="display: block">Show I/O Partitions</button>
        <button id='hide-io-view-button' onclick="hideIoView()" style="display: none">Hide I/O Partitions</button>
    </div>
    <div id='host-instructions' class='instructions-container'>
        <div id='host-instructions-close-button-container' class='instructions-close-button-container'>
            <img id='host-instructions-close-button' class='instructions-close-button' src='public/img/close-button.png' onclick='hideInstructions("host-instructions", "information-img")'/>
        </div>
        <p id='host-instructions-text'>Hover over or click on a host on the legend to isolate that host on the graph. Click on a selected host to deselect it. or just select a different host on the legend</p>
    </div>
    <canvas id="graph-container"></canvas>
`
