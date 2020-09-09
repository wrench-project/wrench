let bandwidthUsageChart = null;

$('#bandwidth-dropdown').dropdown({
    action: function(text, value) {
        let unit = dataSizeUnits.B
        switch (text) {
            case dataSizeUnits.KB[1]:
                unit = dataSizeUnits.KB;
                break;
            case dataSizeUnits.MB[1]:
                unit = dataSizeUnits.MB;
                break;
            case dataSizeUnits.GB[1]:
                unit = dataSizeUnits.GB;
                break;
            case dataSizeUnits.TB[1]:
                unit = dataSizeUnits.TB;
                break;
            case dataSizeUnits.PB[1]:
                unit = dataSizeUnits.PB;
        }
        generateBandwidthUsage(data, unit);
    }
});


function cleanBandwidthUsageChart() {
    if (bandwidthUsageChart !== null) {
        bandwidthUsageChart.destroy();
    }
}

/**
 * Generates the network bandwidth usage.
 *
 * @param rawData: Simulation data.
 * @param unit: Bandwidth usage unit.
 * @param containedId: Id for the chart container element.
 * @param linknames: List of link names to be displayed.
 * @param zoom: Whether to allow zoom functionality in the chart.
 */
function generateBandwidthUsage(rawData,
                                unit = dataSizeUnits.KB,
                                containedId = null,
                                zoom = true,
                                linknames = []) {
    // clean chart
    cleanBandwidthUsageChart();

    const containerId = containedId ? containedId : 'network-bandwidth-usage-chart';
    let ctx = document.getElementById(containerId);

    // prepare data
    let data = {labels: [], datasets: []}
    let zoomMaxRange = 0;

    let keys = Object.keys(rawData.network);
    keys.forEach(function (key) {
        let link = rawData.network[key];
        if (linknames.length > 0 && !linknames.includes(link.linkname)) {
            return;
        }

        let bytes = [];
        for (let idx in link.link_usage_trace) {
            let entry = link.link_usage_trace[idx];
            data.labels.push(entry.time);
            zoomMaxRange = Math.max(zoomMaxRange, entry.time);
            bytes.push(entry["bytes per second"] / Math.pow(10, unit[2]));
        }

        data.datasets.push({
            label: link.linkname,
            fill: true,
            backgroundColor: getRandomColor(),
            data: bytes
        });
    });

    // zoom properties
    let pluginsProperties = definePluginsProperties(zoom, zoomMaxRange);

    bandwidthUsageChart = new Chart(ctx, {
        type: 'line',
        data: data,
        options: {
            scales: {
                yAxes: [{
                    scaleLabel: {
                        display: true,
                        labelString: 'Bandwidth Usage (' + unit[0] + ')'
                    }
                }],
                xAxes: [{
                    scaleLabel: {
                        display: true,
                        labelString: 'Time (seconds)'
                    }
                }]
            },
            tooltips: {
                callbacks: {
                    label: function (tooltipItem, data) {

                    }
                }
            },
            plugins: pluginsProperties
        }
    });
}
