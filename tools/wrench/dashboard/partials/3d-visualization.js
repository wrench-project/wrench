const threedVisualizationHtml = `
    <img id='three-d-information-img' class='information-img' src='public/img/information.png' onclick='showInstructions("three-d-instructions", "three-d-information-img")'/>
    <div id='three-d-instructions' class='instructions-container' style='text-align: left'>
        <div id='three-d-instructions-close-button-container' class='instructions-close-button-container'>
            <img id='three-d-instructions-close-button' class='instructions-close-button' src='public/img/close-button.png' onclick='hideInstructions("three-d-instructions", "three-d-information-img")'/>
        </div>
        <p id='three-d-instructions-text'>
            Each cube in this 3D graph represents a task. The color to task ID mapping can be found in the legend below. Hover over or click a task in the legend to view more details about the task and highlight it in the graph. You can click and drag the graph to adjust the angle. 
        </p>
        <p>
            Use these configurations to change how you view the graph:
        </p>
        <ul>
            <li>Origin: change where the origin is on the page</li>
            <li>Time interval: adjust how much time a square on the grid represents</li>
            <li>Scale: alter the size of of the squares</li>
        </ul>
    </div>
    <div id='three-d-graph-config'>
        <div id='three-d-origin' class='three-d-graph-config'>
            Origin:<br>
            X: <input type='number' id='origin-x'>
            Y: <input type='number' id='origin-y'>
        </div>
        <div id='time-interval-container' class='three-d-graph-config'>
            Time interval: <br>
            <input type='number' id='time-interval' step='10'> <br>
        </div>
        <div id='scale-input-container' class='three-d-graph-config'>
            Scale: <br><input type='number' id='scale-input'>
        </div>
    </div>
    <p>Legend:</p>
    <div class="container legend" id="three-d-legend"></div>
    <div class="text-left" id="three-d-tooltip-container">
        <span id="three-d-tooltip-task-id"></span><br>
        <span id="three-d-tooltip-host"></span><br>
        <span id="three-d-tooltip-task-operation-duration"></span>
    </div>
    <svg width="100%" height="1000px" id='three-d-graph-svg'></svg>
`