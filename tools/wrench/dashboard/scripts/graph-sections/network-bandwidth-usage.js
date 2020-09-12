let bandwidthUsageChart = null;

$("#bandwidth-dropdown").dropdown({
    action: function (text, value) {
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
 * @param zoom: Whether to allow zoom functionality in the chart.
 * @param linknames: List of link names to be displayed.
 * @param range: Range to plot the data.
 */
function generateBandwidthUsage(rawData,
                                unit = dataSizeUnits.KB,
                                containedId = null,
                                zoom = true,
                                linknames = [],
                                range = null) {
    // clean chart
    cleanBandwidthUsageChart();

    const containerId = containedId ? containedId : "network-bandwidth-usage-chart";
    let ctx = document.getElementById(containerId);

    // prepare data
    let data = {labels: [], datasets: []}
    let zoomMaxRange = 0;
    let labels = [];

    let keys = Object.keys(rawData.network);
    keys.forEach(function (key) {
        let link = rawData.network[key];
        if (linknames.length > 0 && !linknames.includes(link.linkname)) {
            return;
        }

        let bytes = [];
        for (let idx in link.link_usage_trace) {
            let entry = link.link_usage_trace[idx];
            if (!range || (range && entry.time >= range[0] && entry.time <= range[1])) {
                if (!labels.includes(entry.time)) {
                    labels.push(entry.time);
                }
                zoomMaxRange = Math.max(zoomMaxRange, entry.time);
                bytes.push(parseFloat(entry["bytes per second"] / Math.pow(10, unit[2])).toFixed(3));
            }
        }

        data.datasets.push({
            label: link.linkname,
            fill: true,
            backgroundColor: getRandomColor(),
            steppedLine: "middle",
            data: bytes
        });
    });
    data.labels = labels;

    // zoom properties
    let pluginsProperties = definePluginsProperties(zoom, zoomMaxRange);

    bandwidthUsageChart = new Chart(ctx, {
        type: "line",
        data: data,
        options: {
            scales: {
                yAxes: [{
                    scaleLabel: {
                        display: true,
                        labelString: "Bandwidth Usage (" + unit[0] + ")"
                    }
                }],
                xAxes: [{
                    scaleLabel: {
                        display: true,
                        labelString: "Time (seconds)"
                    }
                }]
            },
            tooltips: {
                mode: 'index',
                intersect: false
            },
            plugins: pluginsProperties
        }
    });
}
