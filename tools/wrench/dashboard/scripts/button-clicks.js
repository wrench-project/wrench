function toggleDashboard(id) {
    let el = $(`#btn-${id}`);
    let els = document.getElementById(id);

    if (el.hasClass("active")) {
        el.removeClass("active");
        els.style.display = 'none';
        els.style.visibility = 'hidden';
    } else {
        el.addClass("active");
        els.style.display = 'inline-block';
        els.style.visibility = 'visible';
    }
}

function toggleView(obj) {
    if (currGraphState === "taskView") {
        generateHostGanttChart(data, currZoomState["overall-graph-container"]);
        currGraphState = "hostView";
        obj.innerHTML = "<i class=\"exchange icon\"></i> Switch to Task View";
    } else if (currGraphState === "hostView") {
        generateGanttChart(data, null, currZoomState["overall-graph-container"]);
        currGraphState = "taskView";
        obj.innerHTML = "<i class=\"exchange icon\"></i> Switch to Host View";
    }
}

function resizeBox(size, id) {
    let width = size === "full" ? "sixteen" : "eight";
    let otherSize = size === "full" ? "half" : "full";
    document.getElementById(id).className = width + " wide column";
    document.getElementById("dd-width-" + size + "-" + id).className = "check icon";
    document.getElementById("dd-width-" + otherSize + "-" + id).className = "icon";
}

function resizeAllBox(size) {
    let width = size === "full" ? "sixteen" : "eight";
    let otherSize = size === "full" ? "half" : "full";
    for (let i = 0; i < graphInfoArray.length; i++) {
        let id = graphInfoArray[i].containerId;
        document.getElementById(id).className = width + " wide column";
        document.getElementById("dd-width-" + size + "-" + id).className = "check icon";
        document.getElementById("dd-width-" + otherSize + "-" + id).className = "icon";
    }
}

function toggleZoom(id) {
    let zoom = id in currZoomState ? currZoomState[id] : true;
    if (id === "host-utilization-graph-container") {
        generateHostUtilizationChart(data, null, [], [], !zoom);
    } else if (id === "overall-graph-container") {
        if (currGraphState === "taskView") {
            generateGanttChart(data, null, !zoom);
        } else if (currGraphState === "hostView") {
            generateHostGanttChart(data, !zoom);
        }
    } else if (id === "network-graph-container") {
        generateBandwidthUsage(data, dataSizeUnits.KB, null, !zoom, []);
    }
    currZoomState[id] = !zoom;
    document.getElementById("dd-zoom-" + id).className = !zoom ? "check icon" : "icon";
}


function showHideArrow(id, arrowId) {
    var overallDiv = $(`#${id}`)
    var arrow = $(`#${arrowId}`)
    overallDiv.slideToggle()
    if (overallDiv.hasClass('hidden')) {
        arrow.rotate({animateTo: 180})
        overallDiv.removeClass('hidden')
    } else {
        arrow.rotate({animateTo: 0})
        overallDiv.addClass('hidden')
    }
}

function getHostNames(data) {
    var hostNames = new Set();

    data.forEach(function (task) {
        var hostName = task[executionHostKey].hostname
        hostNames.add(hostName)
    })

    hostNames = Array.from(hostNames)
    return hostNames
}

function hostColoursJSONPopulated() {
    for (var key in hostColours) {
        return true
    }
    return false
}

function showIoView() {
    document.getElementById('show-io-view-button').style.display = 'none'
    document.getElementById('hide-io-view-button').style.display = 'block'
    generateGraph(data.contents, currGraphState, true, 1000, 1000)
}

function hideIoView() {
    document.getElementById('show-io-view-button').style.display = 'block'
    document.getElementById('hide-io-view-button').style.display = 'none'
    generateGraph(data.contents, currGraphState, false, 1000, 1000)
}

function legendHover(hostName, id, alreadySelected) {
    if (currentlySelectedHost.hostName !== '') {
        return
    }
    var legendElement = d3.select(`#${id}`)
    if (alreadySelected) {
        legendElement.style("font-weight", "normal")
    } else {
        legendElement.style("font-weight", "bold")
    }
    switchToHostView(data.contents, hostName)
}


function legendClick(hostName, id) {
    var legendElement = d3.select(`#${id}`)
    if (hostName === currentlySelectedHost.hostName) {
        // deselect same host
        currentlySelectedHost.hostName = ""
        currentlySelectedHost.id = ""
        legendElement.style("font-weight", "normal")
        switchToHostView(data.contents, "")
        return
    }
    if (currentlySelectedHost.hostName !== "" && hostName !== currentlySelectedHost.hostName) {
        // deselect different host
        var currentlySelectedLegendElement = d3.select(`#${currentlySelectedHost.id}`)
        currentlySelectedLegendElement.style("font-weight", "normal")
    }
    currentlySelectedHost.hostName = hostName
    currentlySelectedHost.id = id
    legendElement.style("font-weight", "bold")
    switchToHostView(data.contents, hostName)
}

function hideInstructions(instructions, img) {
    var hostInstructions = document.getElementById(instructions)
    var informationImg = document.getElementById(img)
    hostInstructions.style.display = "none"
    informationImg.style.display = "inline-block"
}
