const graphInfoArray = [
    {
        "title": "Simulation Metrics",
        "containerId": "overall-metrics-table-container",
        "html": workflowSummaryHtml,
        "icon": "chart pie"
    },
    {
        "title": "Gantt Chart",
        "containerId": "overall-graph-container",
        "html": simulationGraphHtml,
        "icon": "sitemap"
    },
    {
        "title": "Simulation Details",
        "containerId": "task-details-table-container",
        "html": simulationDetailsHtml,
        "icon": "table"
    },
    {
        "title": "Host Utilization",
        "containerId": "host-utilization-graph-container",
        "html": hostUtilizationHtml,
        "icon": "chart bar"
    },
    {
        "title": "Energy Consumption",
        "containerId": "energy-graph",
        "html": energyGraphsHtml,
        "icon": "lightbulb"
    },
    {
        "title": "3D Visualization",
        "containerId": "three-d-graph-container",
        "html": threedVisualizationHtml,
        "icon": "cubes"
    }
]

function render() {
    const template = document.getElementById('graph-template').innerHTML;
    const templateMenu = document.getElementById('graph-template-menu').innerHTML;
    const renderGraphs = Handlebars.compile(template);
    const renderGraphsMenu = Handlebars.compile(templateMenu);
    document.getElementById('main-body').innerHTML = renderGraphs({graphInfo: graphInfoArray});
    document.getElementById('main-sidebar').innerHTML = renderGraphsMenu({graphInfo: graphInfoArray});
}

render();

$('.ui.dropdown')
    .dropdown();
