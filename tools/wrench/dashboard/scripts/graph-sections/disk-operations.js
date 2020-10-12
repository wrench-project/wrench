const generateVerticalPositions = function(times) {
    const result = [] // array of vertical positions
    times.forEach(function(t) {
        if (result.length === 0) {
            result.push([t]) // create first vertical position
        } else {
            let currVp = 0
            for (let i = 0; i < result.length; i++) { // iterate through vertical position
                const vp = result[i]
                let conflictFound = false
                for (let i = 0; i < vp.length; i++) {
                    const time = vp[i]
                    if ((time.start <= t.start && time.end >= t.start) || (t.end >= time.start && t.end <= time.end)) {
                        currVp++
                        conflictFound = true
                        break
                    }
                }
                if (!conflictFound) { // break if we found a good vertical positionn
                    break
                }
            }
            if (currVp >= result.length) { // create a neww vertical position
                result.push([t])
            } else {
                result[currVp].push(t) // add to existing vertical postiion
            }
        }
    })
    return result
}

const formatForChart = function(readVps, writeVps, colours) {
    const results = []
    let counter = 0
    console.log(readVps, writeVps)
    readVps.forEach(function(rvp, i) {
        const currStack = i + 1
        rvp.forEach(function(time, j) {
            if (results.length === 0) {
                results.push({
                    label: time.host,
                    backgroundColor: colours[time.host],
                    data: [[time.start, time.end], [0,0]], 
                    stack: currStack
                })
            } else {
                let toMinus = 0
                if (currStack === results[counter - 1].stack) {
                    toMinus = rvp[j-1].end
                }
                results.push({
                    label: time.host,
                    backgroundColor: colours[time.host],
                    data: [[time.start - toMinus, time.end - toMinus],[0,0]],
                    stack: currStack
                })
            }
            counter++
        })
    })

    counter = 0
    for (let i = 0; i < writeVps.length; i++) {
        const wvp = writeVps[i]
        const currStack = i + 1
        for (let j = 0; j < wvp.length; j++) {
            let toMinus = 0
            if (counter > 0 && j > 0) {
                toMinus = wvp[j-1].end
            }
            const time = wvp[j]
            const resultStack = results[counter].stack
            if (resultStack === currStack) {
                results[counter].data[1] = [time.start - toMinus, time.end - toMinus]
            } else if (resultStack < currStack) {
                while (results[counter + 1].stack != currStack) {
                    counter++
                }
            } else if (resultStack > currStack) {
                results.splice(counter, 0, {
                    label: time.host,
                    backgroundColor: colours[time.host],
                    data: [[0,0], [time.start - toMinus, time.end - toMinus]],
                    stack: currStack
                })
            }
            counter++
        }
    }
    return results
}

const generateDiskOperations = function(data) {
    const allReads = []
    const allWrites = []
    const colours = {}
    Object.keys(data).forEach(function(host) {
        const { reads, writes } = (data[host])["/"]
        if (reads != null) {
            reads.forEach(function(r) {
                r.host = host
                allReads.push(r)
            })
        }
        if (writes != null) {
            writes.forEach(function(w) {
                w.host = host
                allWrites.push(w)
            })
        }
        colours[host] = getRandomColor()
    })
    const startTimeComparator = function(a, b) {
        return a.start - b.start
    }
    allReads.sort(startTimeComparator)
    allWrites.sort(startTimeComparator)
    console.log(allReads, allWrites)
    readVps = generateVerticalPositions(allReads)
    writeVps = generateVerticalPositions(allWrites)
    const datasets = formatForChart(readVps, writeVps, colours)
    new Chart(document.getElementById("disk-operations"), {
        type: 'horizontalBar',
        data: {
          labels: ["Reads", "Writes"],
          datasets
        },
        options: {
            title: {
                display: true,
                text: 'Population growth (millions)'
            },
            scales: {
                yAxes: [{
                    ticks: {
                        reverse: true
                    },
                    scaleLabel: {
                        display: true,
                        labelString: 'Action'
                    }
                }],
                xAxes: [{
                    stacked: true,
                    scaleLabel: {
                        display: true,
                        labelString: 'Time (seconds)'
                    }
                }]
            }
        }
    });
}

/*
[
    READS
    vp1: [[start1, end1], [start2, end2]]
    vp2: [[start3, end3], [start4, end4]]
    vp3: [[start5, end5], [start6, end6]]

    WRITES
    vp1: [[start7, end7], [start8, end8]]
    vp2: [[start9, end9], [start10, end10]]
    vp3: [[start11, end12], [start13, end13]]
]
*/

/*
    [time1 (stack:1), time7 (stack: 1)]
    [time2 (stack:1), time8 (stack: 1)]
    [time3 (stack:2)]
    [time4 (stack:2)]
*/

 // return [
    //     {
    //         label: "Read",
    //         backgroundColor: getRandomColor(),
    //         data: [[1,2],[5,6]],
    //         stack: 1
    //     },
    //     {
    //         label: "Read",
    //         backgroundColor: getRandomColor(),
    //         data: [[21,22],[0,0]],
    //         stack: 1
    //     },
    //     {
    //         label: "Write",
    //         backgroundColor: getRandomColor(),
    //         data: [[0,0],[2,3]],
    //         stack: 1
    //     },
    //     {
    //         label: "Read1",
    //         backgroundColor: getRandomColor(),
    //         data: [[3,4],[7,8]],
    //         stack: 2
    //     },
    //     {
    //         label: "Write1",
    //         backgroundColor: getRandomColor(),
    //         data: [[4,5],[8,9]],
    //         stack: 2
    //     },
    //     {
    //         label: "Read1",
    //         backgroundColor: getRandomColor(),
    //         data: [[10,11],[12,13]],
    //         stack: 3
    //     },
    //     {
    //         label: "Write1",
    //         backgroundColor: getRandomColor(),
    //         data: [[14,15],[16,17]],
    //         stack: 3
    //     },
    // ]
