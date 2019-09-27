// import { generateGraph } from './graph-sections/simulation-graph'
// import { populateLegend } from './util'
// import { populateWorkflowTaskDataTable } from './graph-sections/simulation-details'
// import { getOverallWorkflowMetrics } from './graph-sections/workflow-summary'
// import { generate3dGraph } from './graph-sections/3d-visualization'

function initialise() {
    var noFileDiv = document.getElementById("no-file")
    var mainBodyDiv = document.getElementById("main-body")
    if (localStorage.getItem("firstVisit") === "null") {
        localStorage.setItem("firstVisit", true)
        firstVisit = true
    }
    if (data.file === undefined) {
        noFileDiv.style.display = "block"
        mainBodyDiv.style.display = "none"
    } else {
        noFileDiv.style.display = "none"
        mainBodyDiv.style.display = "block"
        generateGraph(data.contents, "graph-container")
        populateLegend("taskView")
        populateWorkflowTaskDataTable(data.contents)
        getOverallWorkflowMetrics(data.contents)
        generate3dGraph(data.contents, true)
    }
}