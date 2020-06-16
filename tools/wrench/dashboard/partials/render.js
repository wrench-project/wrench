const graphInfoArray = [
    {
        "title": "Summary of Simulation Metrics",
        "containerId": "overall-metrics-table-container",
        "arrowId": "overall-metrics-arrow",
        "html": workflowSummaryHtml
    },
    {
        "title": "Simulation Graph",
        "containerId": "overall-graph-container",
        "arrowId": "simulation-graph-arrow",
        "html": simulationGraphHtml
    },
    {
        "title": "Simulation Details",
        "containerId": "task-details-table-container",
        "arrowId": "simulation-details-arrow",
        "html": simulationDetailsHtml
    },
    {
        "title": "Host Utilization Graph",
        "containerId": "host-utilization-graph-container",
        "arrowId": "host-utilization-arrow",
        "html": hostUtilizationHtml
    },
    {
        "title": "Energy Graphs",
        "containerId": "energy-graph",
        "arrowId": "energy-graph-arrow",
        "html": energyGraphsHtml
    }, 
    {
        "title": "3D Visualization",
        "containerId": "three-d-graph-container",
        "arrowId": "three-d-graph-arrow",
        "html": threedVisualizationHtml
    }
]

function render() {
    const template = document.getElementById('graph-template').innerHTML
    const renderGraphs = Handlebars.compile(template)
    document.getElementById('main-body').innerHTML = renderGraphs({ graphInfo: graphInfoArray })

}

render()