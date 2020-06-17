const graphInfoArray = [
    {
        "title": "Simulation Metrics",
        "containerId": "overall-metrics-table-container",
        "arrowId": "overall-metrics-arrow",
        "html": workflowSummaryHtml,
        "icon": "chart pie"
    },
    {
        "title": "Gantt Chart",
        "containerId": "overall-graph-container",
        "arrowId": "simulation-graph-arrow",
        "html": simulationGraphHtml,
        "icon": "sitemap"
    },
    {
        "title": "Simulation Details",
        "containerId": "task-details-table-container",
        "arrowId": "simulation-details-arrow",
        "html": simulationDetailsHtml,
        "icon": "table"
    },
    {
        "title": "Host Utilization",
        "containerId": "host-utilization-graph-container",
        "arrowId": "host-utilization-arrow",
        "html": hostUtilizationHtml,
        "icon": "chart bar"
    },
    {
        "title": "Energy Consumption",
        "containerId": "energy-graph",
        "arrowId": "energy-graph-arrow",
        "html": energyGraphsHtml,
        "icon": "lightbulb"
    },
    {
        "title": "3D Visualization",
        "containerId": "three-d-graph-container",
        "arrowId": "three-d-graph-arrow",
        "html": threedVisualizationHtml,
        "icon": "cubes"
    }
]

function render() {
    const template = document.getElementById('graph-template').innerHTML
    const templateMenu = document.getElementById('graph-template-menu').innerHTML
    const renderGraphs = Handlebars.compile(template)
    const renderGraphsMenu = Handlebars.compile(templateMenu)
    document.getElementById('main-body').innerHTML = renderGraphs({graphInfo: graphInfoArray})
    document.getElementById('main-sidebar').innerHTML = renderGraphsMenu({graphInfo: graphInfoArray})
}

render()