function generateConsumedEnergyGraph() {
    eData = [];
    xAxisMarks = ["x"];
    // Iterate through data for each host
    for(var i = 0; i < energyData.length; i++) {
        var hostData = energyData[i]["consumed_energy_trace"];

        consumedEnergyData = []

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
        bindto: "#consumedEnergyGraph",
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
                bindto: "#consumedEnergyGraphLegend",
                template: "<span style='color:#fff;padding:10px;background-color:{=COLOR}'>{=TITLE}</span>"
            }
        }
    });
}

function generatePStateGraph() {
    pStateData = [];
    xAxisMarks = ["x"];
    // Iterate through data for each host
    for(var i = 0; i < energyData.length; i++) {
        var hostData = energyData[i]["pstate_trace"];

        consumedEnergyData = []

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
        bindto: "#pStateGraph",
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
                bindto: "#pStateGraphLegend",
                template: "<span style='color:#fff;padding:10px;background-color:{=COLOR}'>{=TITLE}</span>"
            }
        }
    });
}