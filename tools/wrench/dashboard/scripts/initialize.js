function prepareData(data) {
    const nullReplacement = {
        start: 0,
        end: 0
    };
    data.forEach(function (d) {
        if (d.read === null) {
            d.read = [nullReplacement];
        }
        if (d.compute === null) {
            d.compute = nullReplacement;
        }
        if (d.write === null) {
            d.write = [nullReplacement];
        }
    })
    return data;
}

function initialize() {
    const noFileDiv = document.getElementById("no-file");
    const mainBodyDiv = document.getElementById("main-body");
    const mainMenuDiv = document.getElementById("main-sidebar");
    const energyTitles = document.getElementsByClassName("energy-title");
    const noEnergyFileDiv = document.getElementById("no-energy-file");

    if (localStorage.getItem("firstVisit") === "null") {
        localStorage.setItem("firstVisit", true);
        firstVisit = true;
    }

    if (data.file === undefined && Object.keys(energyData).length === 0) {
        noFileDiv.style.display = "block";
        mainBodyDiv.style.display = "none";
        mainMenuDiv.style.display = "none";
        document.getElementById('main-cog').style.display = "none";
        document.getElementById('main-cog').style.visibility = "hidden";

    } else {
        noFileDiv.style.display = "none";
        mainBodyDiv.style.display = "block";
        mainMenuDiv.style.display = "block";
        document.getElementById('main-cog').style.display = "block";
        document.getElementById('main-cog').style.visibility = "visible";

        if (Object.keys(energyData).length !== 0) {
            Array.from(energyTitles).forEach(function (et) {
                et.style.display = "block";
            });
            noEnergyFileDiv.style.display = "none";
            generateConsumedEnergyGraph(energyData, "consumedEnergyGraph", "consumedEnergyGraphLegend");
            generatePStateGraph(energyData, "pStateGraph", "pStateGraphLegend");

        } else {
            Array.from(energyTitles).forEach(function (et) {
                et.style.display = "none";
            });
            noEnergyFileDiv.style.display = "block";
        }

        if (data.file !== undefined) {
            data.contents = prepareData(data.contents); // TODO: remove
            data.tasks = prepareData(data.tasks);

            generateGanttChart(data);
            generateHostUtilizationChart(data);

            // generateGraph(data.contents, currGraphState, false, 1000, 1000)
            // populateLegend("taskView")
            populateWorkflowTaskDataTable(data.contents)
            getOverallWorkflowMetrics(data.contents)
            generate3dGraph(data.contents, true, true)
            // generateHostUtilizationGraph(data.contents, 1000, 1000, 60)
        }
    }
}

initialize();
