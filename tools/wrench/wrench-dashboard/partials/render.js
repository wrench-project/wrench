const graphInfoArray = [
    {
        "title": "Simulation Metrics",
        "containerId": "overall-metrics-table-container",
        "menuHtml": false,
        "html": workflowSummaryHtml,
        "icon": "chart pie"
    },
    {
        "title": "Gantt Chart",
        "containerId": "overall-graph-container",
        "menuHtml": simulationGraphMenuHtml,
        "html": simulationGraphHtml,
        "icon": "sitemap"
    },
    {
        "title": "Simulation Details",
        "containerId": "task-details-table-container",
        "menuHtml": false,
        "html": simulationDetailsHtml,
        "icon": "table"
    },
    {
        "title": "Host Utilization",
        "containerId": "host-utilization-graph-container",
        "menuHtml": false,
        "html": hostUtilizationHtml,
        "icon": "chart bar"
    },
    {
        "title": "Network Bandwidth Usage",
        "containerId": "network-graph-container",
        "menuHtml": networkBandwidthMenuHtml,
        "html": networkBandwidthHtml,
        "icon": "tachometer alternate"
    },
    {
        "title": "Energy Consumption",
        "containerId": "energy-graph",
        "menuHtml": false,
        "html": energyGraphsHtml,
        "icon": "lightbulb"
    },
    {
        "title": "3D Visualization",
        "containerId": "three-d-graph-container",
        "menuHtml": false,
        "html": threedVisualizationHtml,
        "icon": "cubes"
    },
    {
        "title": "Disk I/O Operations",
        "containerId": "disk-operations-container",
        "html": diskOperationsHtml,
        "icon": ""
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
