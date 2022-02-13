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

const populateChartArray = function(array, isReadVp, results, colours, counter) {
    array.forEach(function (vp, i) { // array is a 2D array, the first level is each vertical position
        const stack = i + 1
        vp.forEach(function(time, j) { // the second level is a time within that vertical position
            let toMinus = 0
            if (results.length > 0 && stack === results[counter - 1].stack) { // minus the last time from the same vertical position because chart js assumes its from the last event not from 0
                toMinus = vp[j-1].end
            }
            results.push({
                label: time.host,
                backgroundColor: colours[time.host],
                data: isReadVp ? [[time.start - toMinus, time.end - toMinus],[0,0]] : [[0,0], [time.start - toMinus, time.end - toMinus]],
                stack
            })
            counter++
        })
    })
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
    readVps = generateVerticalPositions(allReads)
    writeVps = generateVerticalPositions(allWrites)
    let datasets = populateChartArray(readVps, true, [], colours, 0)
    datasets = populateChartArray(writeVps, false, datasets, colours, datasets.length)
    const maxVal = Math.max(allReads[allReads.length-1].end, allWrites[allWrites.length-1].end)
    const pluginsProperties = definePluginsProperties(true, (Math.ceil(maxVal / 100 ) * 100) + 100);
    new Chart(document.getElementById("disk-operations"), {
        type: 'horizontalBar',
        data: {
          labels: ["Reads", "Writes"],
          datasets
        },
        options: {
            title: {
                display: true,
                text: 'Disk I/O Operations'
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
            },
            legend: {
                labels: {
                    filter: function(item, chart) {
                        let matchFoundIndex = -1
                        for (let i = 0; i < chart.datasets.length; i++) {
                            const chartLabel = chart.datasets[i].label
                            if (i === item.datasetIndex) {
                                return matchFoundIndex === -1
                            }
                            if (chartLabel === item.text) {
                                matchFoundIndex = i
                            }
                        }
                    }
                }
            },
            tooltips: {
                position: 'nearest',
                mode: 'point',
                intersect: 'false',
                callbacks: {
                    label: function (tooltipItem, data) {
                        return data.datasets[tooltipItem.datasetIndex].label
                    },
                    afterBody: function (tooltipItem, data) {
                        const { stack } = data.datasets[tooltipItem[0].datasetIndex]
                        const readWrite = tooltipItem[0].yLabel
                        let toMinus = 0
                        for (let i = 0; i < stack - 1; i++) {
                            toMinus += (readWrite === "Reads" ? readVps[i].length : writeVps[i].length)
                        }
                        toMinus += readWrite === "Writes" ? allReads.length : 0
                        const vpIndex = tooltipItem[0].datasetIndex - toMinus
                        const { bytes } = readWrite === "Reads" ? readVps[stack-1][vpIndex] : writeVps[stack-1][vpIndex]

                        let [start, end] = tooltipItem[0].xLabel.split(",")
                        start = start.replace("[","")
                        start = start.trim()
                        end = end.replace("]", "")
                        end = end.trim()

                        return `${bytes} bytes read\nDuration: ${end - start}`
                    }
                }
            },
            plugins: pluginsProperties
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
