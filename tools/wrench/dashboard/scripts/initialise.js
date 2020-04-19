function prepareData(data) {
    const nullReplacement = {
        start: 0,
        end: 0
    }
    data.forEach(function (d) {
        if (d.read === null) {
            d.read = [nullReplacement]
        }
        if (d.compute === null) {
            d.compute = nullReplacement
        }
        if (d.write === null) {
            d.write = [nullReplacement]
        }
    })
    return data
}

function initialise() {
    const noFileDiv = document.getElementById("no-file")
    const mainBodyDiv = document.getElementById("main-body")
    const energyTitles = document.getElementsByClassName("energy-title")
    const noEnergyFileDiv = document.getElementById("no-energy-file")
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
        if (Object.keys(energyData).length !== 0) {
            Array.from(energyTitles).forEach(function (et) {
                et.style.display = "block"
            })
            noEnergyFileDiv.style.display = "none"
            generateConsumedEnergyGraph(energyData, "consumedEnergyGraph", "consumedEnergyGraphLegend")
            generatePStateGraph(energyData, "pStateGraph", "pStateGraphLegend")
        } else {
            Array.from(energyTitles).forEach(function (et) {
                et.style.display = "none"
            })
            noEnergyFileDiv.style.display = "block"
        }
        if (data.file !== undefined) {
            data.contents = prepareData(data.contents)
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
