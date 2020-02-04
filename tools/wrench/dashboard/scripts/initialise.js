function initialise() {
    if (energyData != []) {
        generateConsumedEnergyGraph(energyData, "consumedEnergyGraph", "consumedEnergyGraphLegend")
        generatePStateGraph(energyData, "pStateGraph", "pStateGraphLegend")
    }
    var noFileDiv = document.getElementById("no-file")
    var mainBodyDiv = document.getElementById("main-body")
    if (localStorage.getItem("firstVisit") === "null") {
        localStorage.setItem("firstVisit", true)
        firstVisit = true
    }
    if (data.file === undefined && Object.keys(energyData).length === 0) {
        noFileDiv.style.display = "block"
        mainBodyDiv.style.display = "none"
    } else {
        noFileDiv.style.display = "none"
        mainBodyDiv.style.display = "block"
        if (data.file !== undefined) {
            generateGraph(data.contents, "graph-container", currGraphState, 1000, 1000)
            populateLegend("taskView")
            populateWorkflowTaskDataTable(data.contents, "task-details-table", "task-details-table-body", "task-details-table-td")
            getOverallWorkflowMetrics(data.contents, "overall-metrics-table", "task-details-table-td")
            generate3dGraph(data.contents, true, true, "three-d-graph-svg", "origin-x", "origin-y", "time-interval", "scale-input")
            generateHostUtilizationGraph(data.contents, "host-utilization-chart", "host-utilization-chart-tooltip", "host-utilization-chart-tooltip-task-id", "host-utilization-chart-tooltip-compute-time", 1000, 1000)
        }
    }
}

initialise()