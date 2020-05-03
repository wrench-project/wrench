var origin, startAngle, xAngle, yAngle
var taskOverlap, maxTaskOverlap
var maxTime, timeScalingFactor
var scale
var svg
var cubesGroup
var originXBox, originYBox, timeIntervalBox, scaleBox
var color = d3.scaleOrdinal(d3.schemeCategory20)
var mx, my, mouseX, mouseY
var threeDColourMap = {}
var currentlySelectedCube
var cubeIsClicked = false
var grid3d, scale3d, cubes3d
var xGrid, yLine, cubesData, xLine, ftCubesData

function initialise3dGraph() {
    origin = [0, 200]
    startAngle = Math.PI / 4
    yAngle = startAngle
    xAngle = -startAngle
    taskOverlap = determineTaskOverlap(data.contents)
    maxTaskOverlap = Object.keys(taskOverlap).length + 1
    maxTime = d3.max(data.contents, function (d) {
        return Math.max(d['whole_task'].end, d['failed'], d['terminated'])
    })
    timeScalingFactor = maxTime / 32 // 32 is the ideal number of columns for the width of the space
    if (timeScalingFactor < 10) {
        timeScalingFactor = 10
    } else {
        timeScalingFactor = Math.round(timeScalingFactor / 10) * 10 // round to the nearest 10
    }
    scale = maxTaskOverlap * 2
    grid3d = d3._3d()
        .shape('GRID', maxTaskOverlap)
        .origin(origin)
        .rotateY(startAngle)
        .rotateX(-startAngle)
        .scale(scale)
    scale3d = d3._3d()
        .shape('LINE_STRIP')
        .origin(origin)
        .rotateY(startAngle)
        .rotateX(-startAngle)
        .scale(scale)
    cubes3d = d3._3d()
        .shape('CUBE')
        .x(function (d) {
            return d.x;
        })
        .y(function (d) {
            return d.y;
        })
        .z(function (d) {
            return d.z;
        })
        .rotateY(startAngle)
        .rotateX(-startAngle)
        .origin(origin)
        .scale(scale)
}

function changeOriginOrScale(newOrigin, newScale) {
    origin = newOrigin
    scale = newScale
    grid3d = d3._3d()
        .shape('GRID', maxTaskOverlap)
        .origin(origin)
        .rotateY(yAngle)
        .rotateX(xAngle)
        .scale(scale)

    scale3d = d3._3d()
        .shape('LINE_STRIP')
        .origin(origin)
        .rotateY(yAngle)
        .rotateX(xAngle)
        .scale(scale)

    cubes3d = d3._3d()
        .shape('CUBE')
        .x(function (d) {
            return d.x;
        })
        .y(function (d) {
            return d.y;
        })
        .z(function (d) {
            return d.z;
        })
        .rotateY(yAngle)
        .rotateX(xAngle)
        .origin(origin)
        .scale(scale)

    var newOriginData = [
        grid3d(xGrid),
        scale3d([yLine]),
        cubes3d(cubesData),
        scale3d([xLine]),
        data.contents,
        cubes3d(ftCubesData)
    ]
    processData(newOriginData, 1000, false)
}

function changeTimeScalingFactor(factor) {
    timeScalingFactor = factor
    generate3dGraph(data.contents, false, false)
}

function showInstructions(instructions, img) {
    var hostInstructions = document.getElementById(instructions)
    var informationImg = document.getElementById(img)
    hostInstructions.style.display = "block"
    informationImg.style.display = "none"
}

function determineMaxNumCoresAllocated(data) {
    var max = 0
    data.forEach(function (d) {
        if (d.num_cores_allocated >= max) {
            max = d.num_cores_allocated
        }
    })
    return max
}

function makeCube(h, x, z, duration, colour, taskId) {
    return [
        {x: x, y: h, z: z + duration}, // FRONT TOP LEFT
        {x: x, y: 0, z: z + duration}, // FRONT BOTTOM LEFT
        {x: x + 1, y: 0, z: z + duration}, // FRONT BOTTOM RIGHT
        {x: x + 1, y: h, z: z + duration}, // FRONT TOP RIGHT
        {x: x, y: h, z: z}, // BACK  TOP LEFT
        {x: x, y: 0, z: z}, // BACK  BOTTOM LEFT
        {x: x + 1, y: 0, z: z}, // BACK  BOTTOM RIGHT
        {x: x + 1, y: h, z: z}, // BACK  TOP RIGHT
        {colour},
        {taskId}
    ];
}

function dragged() {
    mouseX = mouseX || 0;
    mouseY = mouseY || 0;
    var beta = (d3.event.x - mx + mouseX) * Math.PI / 230;
    var alpha = (d3.event.y - my + mouseY) * Math.PI / 230 * (-1);
    yAngle = beta + startAngle
    xAngle = alpha - startAngle
    var rotatedData = [
        grid3d.rotateY(beta + startAngle).rotateX(alpha - startAngle)(xGrid),
        scale3d.rotateY(beta + startAngle).rotateX(alpha - startAngle)([yLine]),
        cubes3d.rotateY(beta + startAngle).rotateX(alpha - startAngle)(cubesData),
        scale3d.rotateY(beta + startAngle).rotateX(alpha - startAngle)([xLine]),
        data.contents,
        cubes3d.rotateY(beta + startAngle).rotateX(alpha - startAngle)(ftCubesData)
    ];
    processData(rotatedData, 0, false);
}

function dragStart() {
    mx = d3.event.x;
    my = d3.event.y;
}

function dragEnd() {
    mouseX = d3.event.x - mx + mouseX;
    mouseY = d3.event.y - my + mouseY;
}

function sort3dLegend() {
    var legend = document.getElementById('three-d-legend')
    var legendItems = Array.from(legend.children)
    var ftItems = []
    legendItems.sort(function (a, b) {
        return a.innerHTML.toLowerCase().localeCompare(b.innerHTML.toLowerCase(), undefined, {
            numeric: true,
            sensitivity: 'base'
        })
    })
    legend.innerHTML = ''
    legendItems.forEach(function (l) {
        if (l.innerHTML === "Failed During Execution" || l.innerHTML === "Terminated by User") {
            ftItems.push(l)
        }
        legend.appendChild(l)
    })
    ftItems.forEach(function (l) {
        legend.appendChild(l)
    })
}

function showAndPopulateTooltip(d) {
    var toolTipContainer = document.getElementById('three-d-tooltip-container')
    toolTipContainer.style.visibility = 'visible'


    var tooltipTaskId = d3.select('#three-d-tooltip-task-id')
    var tooltipHost = d3.select('#three-d-tooltip-host')
    var tooltipDuration = d3.select('#three-d-tooltip-task-operation-duration')

    tooltipTaskId.text('TaskID: ' + d.task_id)

    tooltipHost.text('Host Name: ' + d['execution host'].hostname)

    var durationFull = findDuration(data.contents, d.task_id, "whole_task")
    tooltipDuration.text('Duration: ' + toFiveDecimalPlaces(durationFull) + 's')
}

function hideTooltip() {
    var toolTipContainer = document.getElementById('three-d-tooltip-container')
    toolTipContainer.style.visibility = 'hidden'
}

function selectCube(taskId, hover) {
    if (currentlySelectedCube == taskId && !hover) {
        deselectCube()
        return
    }
    var currData = data.contents
    if (!hover) {
        currentlySelectedCube = taskId
    }
    currData.forEach(function (d) {
        if (d.task_id === taskId) {
            document.getElementById(`three-d-legend-${taskId}`).style.fontWeight = 'bold'
            document.getElementById(`cube-${taskId}`).style.fill = threeDColourMap[taskId]
            var ftCube = document.getElementById(`cube-ft-${taskId}`)
            if (ftCube !== null) {
                ftCube.style.visibility = 'visible'
            }
            showAndPopulateTooltip(d)
        } else {
            document.getElementById(`three-d-legend-${d.task_id}`).style.fontWeight = 'normal'
            document.getElementById(`cube-${d.task_id}`).style.fill = 'gray'
            var ftCube = document.getElementById(`cube-ft-${d.task_id}`)
            if (ftCube !== null) {
                ftCube.style.visibility = 'hidden'
            }
        }
    })
}

function deselectCube(hover) {
    if (!hover) {
        currentlySelectedCube = ''
    }
    var currData = data.contents
    currData.forEach(function (d) {
        document.getElementById(`cube-${d.task_id}`).style.fill = threeDColourMap[d.task_id]
        document.getElementById(`three-d-legend-${d.task_id}`).style.fontWeight = 'normal'
        var ftCube = document.getElementById(`cube-ft-${d.task_id}`)
        if (ftCube !== null) {
            ftCube.style.visibility = 'visible'
        }
    })
    hideTooltip()
}

function processData(data, tt, populateLegend) {
    /* ----------- GRID ----------- */
    xGrid = svg.selectAll('path.grid').data(data[0], d => { return d.task_id });

    xGrid
        .enter()
        .append('path')
        .attr('class', '_3d grid')
        .merge(xGrid)
        .attr('stroke', 'black')
        .attr('stroke-width', 0.3)
        .attr('fill-opacity', 0)
        .attr('d', grid3d.draw);

    xGrid.exit().remove();

    /* ----------- y-Scale ----------- */

    var yScale = svg.selectAll('path.yScale').data(data[1]);

    yScale
        .enter()
        .append('path')
        .attr('class', '_3d yScale')
        .merge(yScale)
        .attr('stroke', 'black')
        .attr('stroke-width', .5)
        .attr('d', scale3d.draw)

    yScale.exit().remove();

    /* ----------- y-Scale Text ----------- */

    var yText = svg.selectAll('text.yText').data(data[1][0]);

    yText
        .enter()
        .append('text')
        .attr('class', '_3d yText')
        .attr('dx', '.3em')
        .merge(yText)
        .each(function (d) {
            d.centroid = {x: d.rotated.x, y: d.rotated.y, z: d.rotated.z};
        })
        .attr('x', function (d) {
            return d.projected.x;
        })
        .attr('y', function (d) {
            return d.projected.y;
        })
        .attr('font-size', '0.5em')
        .text(function (d, i) {
            if (i + 1 === (data[1][0]).length) {
                return "Cores Allocated"
            }
            if (d[1] <= 0) {
                return d[1] - 1
            } else {
                return ''
            }
        })

    yText.exit().remove();

    /* ----------- x-Scale ----------- */

    var xScale = svg.selectAll('path.xScale').data(data[3]);

    xScale
        .enter()
        .append('path')
        .attr('class', '_3d xScale')
        .merge(xScale)
        .attr('stroke', 'black')
        .attr('stroke-width', .5)
        .attr('d', scale3d.draw)

    xScale.exit().remove();

    /* ----------- x-Scale Text ----------- */

    var xText = svg.selectAll('text.xText').data(data[3][0]);

    xText
        .enter()
        .append('text')
        .attr('class', '_3d xText')
        .attr('dx', '.3em')
        .merge(xText)
        .each(function (d) {
            d.centroid = {x: d.rotated.x, y: d.rotated.y, z: d.rotated.z};
        })
        .attr('x', function (d) {
            return d.projected.x;
        })
        .attr('y', function (d) {
            return d.projected.y;
        })
        .attr('font-size', '0.5em')
        .text(function (d) {
            return d[3]
        })

    xText.exit().remove();

    /* --------- CUBES ---------*/

    var cubes = cubesGroup.selectAll('g.cube').data(data[2], function (d) {
        return d.id
    });

    var legend = d3.select('#three-d-legend')

    if (populateLegend) {
        legend.append("small")
            .attr("class", "inline-block")
            .style("border-left", "15px solid orange")
            .text("Terminated by User")

        legend.append("small")
            .attr("class", "inline-block")
            .style("border-left", "15px solid red")
            .text("Failed During Execution")
    }

    var ce = cubes
        .enter()
        .append('g')
        .attr('class', 'cube')
        .attr('id', function (d, i) {
            return `cube-${data[4][i].task_id}`
        })
        .attr('fill', function (d, i) {
            var colour = getRandomColour()
            threeDColourMap[data[4][i].task_id] = colour
            legend.append("small")
                .attr("class", "inline-block")
                .attr("id", `three-d-legend-${data[4][i].task_id}`)
                .style("border-left", `15px solid ${colour}`)
                .text(data[4][i].task_id)
                .on('click', function () {
                    selectCube(data[4][i].task_id, false)
                    if (currentlySelectedCube === '') {
                        cubeIsClicked = false
                    } else {
                        cubeIsClicked = true
                    }
                })
                .on('mouseover', function () {
                    selectCube(data[4][i].task_id, true)
                })
                .on('mouseout', function () {
                    if (!cubeIsClicked) {
                        deselectCube(true)
                    } else {
                        selectCube(currentlySelectedCube, true)
                    }
                })
            return colour
        })
        .attr('stroke', function (d) {
            return d3.color(color(d.id));
        })
        .merge(cubes)

    sort3dLegend()

    cubes.exit().remove();

    /* --------- FACES ---------*/

    var faces = cubes.merge(ce).selectAll('path.face').data(function (d) {
        return d.faces;
    }, function (d) {
        return d.face;
    });

    faces.enter()
        .append('path')
        .attr('class', 'face')
        .attr('fill-opacity', 0.95)
        .classed('_3d', true)
        .merge(faces)
        .transition().duration(tt)
        .attr('d', cubes3d.draw)

    faces.exit().remove();

    /* --------- FT CUBES ---------*/

    var ftCubes = cubesGroup.selectAll('g.cube').data(data[5], function (d) {
        return d.id
    });

    var ftCE = ftCubes
        .enter()
        .append('g')
        .attr('class', 'cube')
        .attr('id', function (d, i) {
            return `cube-ft-${d[9].taskId}`
        })
        .attr('fill', function (d, i) {
            return d[8].colour
        })
        .attr('stroke', function (d) {
            return d3.color(color(d.id));
        })
        .merge(cubes)

    /* --------- FT FACES ---------*/

    var ftFaces = ftCubes.merge(ftCE).selectAll('path.face').data(function (d) {
        return d.faces;
    }, function (d) {
        return d.face;
    });

    ftFaces.enter()
        .append('path')
        .attr('class', 'face')
        .attr('fill-opacity', 0.95)
        .classed('_3d', true)
        .merge(ftFaces)
        .transition().duration(tt)
        .attr('d', cubes3d.draw)

}

/*
    data: data to generate graph in json array
    populateLegend: pass in true, this is for initial dashboard
    needToInitialse: whether you want to reset all of the parameters for the 3d graph
    svgId: id of the <svg> that will contain the graph
    originXId: id of the <input> element that has the value for the X origin 
    originyId: id of the <input> element that has the value for the Y origin
    timeIntervalId: id of the <input> element that has the value for the time interval on the time axis of the graph
    scaleInputId: id of the <input> element that has the value for the scale of the Z axis
*/
function generate3dGraph(data, populateLegend, needToInitialise, svgId, originXId, originYId, timeIntervalId, scaleInputId) {
    if (needToInitialise) {
        initialise3dGraph()
    }

    if (svgId) {
        svg = d3.select(`#${svgId}`).call(d3.drag().on('drag', dragged).on('start', dragStart).on('end', dragEnd)).append('g')
        cubesGroup = svg.append('g').attr('class', 'cubes')
    }

    if (originXId) {
        originXBox = document.getElementById(originXId)
    }

    if (originYId) {
        originYBox = document.getElementById(originYId)
    }

    if (timeIntervalId) {
        timeIntervalBox = document.getElementById(timeIntervalId)
    }

    if (scaleInputId) {
        scaleBox = document.getElementById(scaleInputId)
    }

    originXBox.value = origin[0]
    originYBox.value = origin[1]
    timeIntervalBox.value = timeScalingFactor
    scaleBox.value = scale

    originXBox.onchange = function (e) {
        changeOriginOrScale([parseFloat(e.target.value), origin[1]], scale)
    }

    originYBox.onchange = function (e) {
        changeOriginOrScale([origin[0], parseFloat(e.target.value)], scale)
    }

    timeIntervalBox.onchange = function (e) {
        changeTimeScalingFactor(parseFloat(e.target.value))
    }

    scaleBox.onchange = function (e) {
        changeOriginOrScale(origin, e.target.value)
    }

    if (firstVisit) {
        showInstructions("three-d-instructions", "three-d-information-img")
    }

    xGrid = [], yLine = [], xLine = []
    for (var z = 0; z <= maxTime + timeScalingFactor; z += timeScalingFactor) {
        for (var x = 0; x < maxTaskOverlap; x++) {
            xGrid.push([x, 1, z / timeScalingFactor])
        }
    }

    var maxNumCoresAllocated = determineMaxNumCoresAllocated(data)
    d3.range(-1, maxNumCoresAllocated + 1, 1).forEach(function (d) {
        yLine.push([0, -d, 0])
    })

    d3.range(0, maxTime, timeScalingFactor).forEach(function (d) {
        xLine.push([0, 1, d / timeScalingFactor, d])
    })
    xLine.push([0, 1, (maxTime / timeScalingFactor) + 1, "Time (seconds)"]) // for axis label

    cubesData = []
    ftCubesData = []
    data.forEach(function (d) {
        var h = d.num_cores_allocated
        var z = d.whole_task.start / timeScalingFactor
        var x = searchOverlap(d.task_id, taskOverlap)
        var duration = (determineTaskEnd(d) - d.whole_task.start) / timeScalingFactor
        var cube = makeCube(h, parseInt(x), z, duration)
        cube.height = h
        cube.id = d.task_id
        cubesData.push(cube)
        if (d.failed !== -1) {
            var failedCube = makeCube(h, parseInt(x), z + duration, 0.00001, "red", d.task_id)
            ftCubesData.push(failedCube)
        }
        if (d.terminated !== -1) {
            var terminatedCube = makeCube(h, parseInt(x), z + duration, 0.00001, "orange", d.task_id)
            ftCubesData.push(terminatedCube)
        }
    })

    var threeDdata = [
        grid3d(xGrid),
        scale3d([yLine]),
        cubes3d(cubesData),
        scale3d([xLine]),
        data,
        cubes3d(ftCubesData)
    ];
    processData(threeDdata, 1000, populateLegend);
}