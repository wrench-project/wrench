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

    data.forEach(function(task) {
        var hostName = task.execution_host.hostname
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

function switchToHostView(data, selectedHost) {
    var hostNames = getHostNames(data)

    if (!hostColoursJSONPopulated()) {
        hostNames.forEach(function(hostName) {
            var colour = getRandomColour()
            while (colour === '#FF0000' || colour === '#FFA500') {
                colour = getRandomColour()
            }
            hostColours[hostName] = colour
        })
    }
    

    data.forEach(function(task) {
        var hostName = task.execution_host.hostname
        var taskRead = d3.select(`#${task.task_id} .read`)
        var taskCompute = d3.select(`#${task.task_id} .compute`)
        var taskWrite = d3.select(`#${task.task_id} .write`)

        taskRead.style("fill", hostColours[hostName])
        taskRead.style("opacity", 1)

        taskCompute.style("fill", hostColours[hostName])
        taskCompute.style("opacity", 1)

        taskWrite.style("fill", hostColours[hostName])
        taskWrite.style("opacity", 1)

        if (selectedHost !== '' && selectedHost !== hostName) {
            taskRead.style("fill", "gray")
            taskRead.style("opacity", 0.2)

            taskCompute.style("fill", "gray")
            taskCompute.style("opacity", 0.2)

            taskWrite.style("fill", "gray")
            taskWrite.style("opacity", 0.2)
        }
    })
}

function toggleView() {
    var hostInstructions = document.getElementById("host-instructions")
    var informationImg = document.getElementById("information-img")
    if (currGraphState === "taskView") {
        switchToHostView(data.contents, '')
        d3.select("#y-axis-label").text("Host Name")
        populateLegend("hostView")
        d3.select("#toggle-view-button").text("Switch to Task View")
        if (firstVisit) {
            hostInstructions.style.display = "block"
            informationImg.style.display = "none"
        } else {
            hostInstructions.style.display = "none"
            informationImg.style.display = "inline-block"
        }
        currGraphState = "hostView"
    } else if (currGraphState === "hostView") {
        generateGraph(data.contents, "graph-container", "hostView", 1000, 1000)
        d3.select("#y-axis-label").text("TaskID")
        populateLegend("taskView")
        d3.select("#toggle-view-button").text("Switch to Host View")
        hostInstructions.style.display = "none"
        informationImg.style.display = "none"
        currGraphState = "taskView"
    }
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