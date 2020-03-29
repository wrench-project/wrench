/*
    energyData: energyData to feed into the function
    graphId: id of the <div> that will contain the graph
    legendId: id of the <div> that will contain the graph
*/
function generateConsumedEnergyGraph(energyData, graphId, legendId) {
    var eData = [];
    var xAxisMarks = ["x"];
    // Iterate through data for each host
    for(var i = 0; i < energyData.length; i++) {
        var hostData = energyData[i]["consumed_energy_trace"];

        var consumedEnergyData = []

        // Iterate through each energy trace for a host
        for(var j = 0; j < hostData.length; j++) {
            var trace = hostData[j];

            if (xAxisMarks.length <= hostData.length) {
                xAxisMarks.push(trace["time"])
            }

            consumedEnergyData.push(trace["joules"])
        }

        // Add host name to the front of the data for graph labeling
        consumedEnergyData.unshift(energyData[i]["hostname"])
        eData.push(consumedEnergyData)
    }
    
    eData.push(xAxisMarks)

    bb.generate({
        bindto: `#${graphId}`,
        data: {
            x : "x",
            columns: eData,
        },
        axis: {
            x: {
                min: 0,
                padding: {
                    left: 0.5,
                    right: 0.5,
                },
                label: "Time",
                tick: {
                    count: xAxisMarks.length,
                    values: xAxisMarks,
                },
            },
            y: {
                label: "Energy Consumed (joules)",
            }
        },
        legend: {
            show: true,
            position: "right",
            contents: {
                bindto: `#${legendId}`,
                template: "<span style='color:#fff;padding:10px;background-color:{=COLOR}'>{=TITLE}</span>"
            }
        }
    });
}

/*
    energyData: energyData to feed into the function
    graphId: id of the HTML element that will contain the graph
    legendId: id of the HTML element that will contain the graph
*/
function generatePStateGraph(energyData, graphId, legendId) {
    var pStateData = [];
    var xAxisMarks = ["x"];
    // Iterate through data for each host
    for(var i = 0; i < energyData.length; i++) {
        var hostData = energyData[i]["pstate_trace"];

        var consumedEnergyData = []

        // Iterate through each energy trace for a host
        for(var j = 0; j < hostData.length; j++) {
            var trace = hostData[j]

            if (xAxisMarks.length <= hostData.length) {
                xAxisMarks.push(trace["time"])
                // console.log(trace["time"])
            }

            consumedEnergyData.push(trace["pstate"])
        }

        // Add host name to the front of the data for graph labeling
        consumedEnergyData.unshift(energyData[i]["hostname"])
        pStateData.push(consumedEnergyData)
    }
    
    pStateData.push(xAxisMarks)

    bb.generate({
        // bindto: "#pStateGraph",
        bindto: `#${graphId}`,
        data: {
            x : "x",
            columns: pStateData,
        },
        axis: {
            x: {
                min: 0,
                padding: {
                    left: 0.5,
                    right: 0.5,
                },
                label: "Time",
                tick: {
                    count: xAxisMarks.length,
                    values: xAxisMarks,
                },
            },
            y: {
                label: "Power State",
            }
        },
        legend: {
            show: true,
            position: "right",
            contents: {
                // bindto: "#pStateGraphLegend",
                bindto: `#${legendId}`,
                template: "<span style='color:#fff;padding:10px;background-color:{=COLOR}'>{=TITLE}</span>"
            }
        }
    });
}