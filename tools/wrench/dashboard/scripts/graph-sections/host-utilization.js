function determineNumCores(data) {
    var numCores = 0;
    var taskOverlap = determineTaskOverlap(data);
    for (var i = 0; i < Object.keys(taskOverlap).length; i++) {
        var currOverlap = taskOverlap[i];
        var maxCores = 0;
        for (var j = 0; j < currOverlap.length; j++) {
            var t = currOverlap[j];
            if (t.num_cores_allocated > maxCores) {
                maxCores = t.num_cores_allocated;
            }
        }
        numCores += maxCores;
    }
    return numCores;
}

function getComputeTime(d) {
    if (d["compute"].start != -1) {
        if (d["compute"].end == -1) {
            if (d.terminated != -1) {
                return d.terminated - d["compute"].start
            } else if (d.failed != -1){
                return d.failed - d["compute"].start
            }
        } else {
            return d["compute"].end - d["compute"].start
        }
    }
    return 0 //Box shouldn't be displayed if start is -1
}

function generateHostUtilizationGraph(data, containerId, tooltipId, tooltipTaskId, tooltipComputeTime, CONTAINER_WIDTH, CONTAINER_HEIGHT) {
    var num_cores = determineNumCores(data);
    var container = d3.select(`#${containerId}`);
    document.getElementById(containerId).innerHTML = hostUtilizationHtml;
    var chart = document.getElementById(containerId);
    const PADDING = 60;

    var svg = d3.select("svg");

    if (svg.empty()) {
        svg.remove();
    }

    var tooltip                         = d3.select(`#${tooltipId}`);
    var tooltip_task_id                 = d3.select(`#${tooltipTaskId}`);
    var tooltip_compute_time            = d3.select(`#${tooltipComputeTime}`);

    svg = container.append("svg")
        .attr("width", CONTAINER_WIDTH)
        .attr("height", CONTAINER_HEIGHT);

    var x_scale = d3.scaleLinear()
        .domain([0, d3.max(data, function(d) {
            return d3.max([d.whole_task.end, d.terminated, d.failed]);
        })])
        .range([PADDING, CONTAINER_WIDTH - PADDING]);

    var tasks_by_hostname = d3.nest()
        .key(function(d) { return d['execution_host'].hostname; })
        .sortKeys(d3.ascending)
        .entries(data);

    var y_hosts = d3.scaleBand()
        .domain(tasks_by_hostname.map(function(d) {
            return d.key;
        }))
        .range([CONTAINER_HEIGHT - PADDING, 10])
        .padding(0.1);

    var y_cores_per_host = d3.map();

    if (num_cores === 0) {
        tasks_by_hostname.forEach(function (d) {
            y_cores_per_host.set(d.key,
                d3.scaleLinear()
                    .domain([0, d.values[0]['execution_host'].cores])
                    .range([y_hosts(d.key) + y_hosts.bandwidth(), y_hosts(d.key)])
            );
        });
    } else {
        tasks_by_hostname.forEach(function (d) {
            y_cores_per_host.set(d.key,
                d3.scaleLinear()
                    .domain([0, num_cores])
                    .range([y_hosts(d.key) + y_hosts.bandwidth(), y_hosts(d.key)])
            );
        });
    }

    svg.append('g').selectAll('rect')
        .data(y_cores_per_host.keys())
        .enter()
        .append("rect")
        .attr("x", PADDING)
        .attr("y", function(d) {
            return y_hosts(d);
        })
        .attr("width", CONTAINER_WIDTH - PADDING - PADDING)
        .attr("height", y_hosts.bandwidth())
        .attr("opacity", 0.3)
        .attr("fill", "#ffd8f8");


    svg.append('g').selectAll("rect")
        .data(data)
        .enter()
        .append("rect")
        .attr("x", function(d) {
            if (d.compute.start === -1) {
                return 0
            }
            return x_scale(d.compute.start);
        })
        .attr("y", function(d) {
            var y_scale = y_cores_per_host.get(d['execution_host'].hostname);
            var vertical_position = searchOverlap(d.task_id, determineTaskOverlap(data))
            return y_scale(vertical_position);
        })
        .attr("width", function(d) {
            if (d.compute.start === -1) {
                return 0
            }
            if (d.compute.end === -1) {
                return x_scale(determineTaskEnd(d)) - x_scale(d.compute.start)
            }
            return x_scale(d.compute.end) - x_scale(d.compute.start);
        })
        .attr("height", function(d) {
            var y_scale = y_cores_per_host.get(d['execution_host'].hostname);
            return y_scale(0) - y_scale(d.num_cores_allocated);
        })
        .attr("fill", "#f7daad")
        .attr("stroke", "gray")
        .on('mouseover', function() {
            tooltip.style('display', 'inline');

            d3.select(this)
                .attr('fill', '#ffdd7f');
        })
        .on('mousemove', function(d) {
            var offset = getOffset(chart, d3.mouse(this));
            var x = window.scrollX + offset.left + 20
            var y = window.scrollY + offset.top - 30 // 20 se

            tooltip.style('left', x + 'px')
                .style('top', y + 'px');

            tooltip_task_id.text('TaskID: ' + d.task_id);
            tooltip_compute_time.text('Compute Time: ' + getComputeTime(d) + 's');
        })
        .on('mouseout', function(d) {
            tooltip.style('display', 'none');

            d3.select(this)
                .attr('fill', '#f7daad')
        });


    var x_axis = d3.axisBottom(x_scale)
        .ticks(d3.max(data, function(d) {
            return d.end;
        }));

    svg.append("g")
        .attr("class", "x-axis")
        .attr("transform",
            "translate(0," + (CONTAINER_HEIGHT - PADDING) + ")")
        .call(x_axis);

    var y_axis = d3.axisLeft(y_hosts).tickSize(0);

    svg.append("g")
        .attr("class", "y-axis")
        .attr("stroke-width", "0px")
        .attr("transform",
            "translate(20, 0)")
        .call(y_axis)
        .selectAll("text")
        .attr("transform", "rotate(-90)")
        .attr("text-anchor", "middle");

    y_cores_per_host.entries().forEach(function(entry) {
        var axis = d3.axisLeft(entry.value)
            .tickValues(d3.range(0, entry.value.domain()[1] + 1, 1))
            .tickFormat(d3.format(""));;

        svg.append("g")
            .attr("class", "y-axis2")
            .attr("transform",
                "translate(" + PADDING + ",0)")
            .call(axis);
    });

    svg.append("text")
        .attr("transform",
            "translate(" + (CONTAINER_WIDTH / 2) + " ," + (CONTAINER_HEIGHT - 10) + ")")
        .style("text-anchor", "middle")
        .attr('font-size', 12+'px')
        .attr('fill', 'gray')
        .text("Time (seconds)");

    svg.append("text")
        .attr("transform",
            "rotate(-90)")
        .attr("y", -3)
        .attr("x", 0 - (CONTAINER_HEIGHT / 2))
        .attr("dy", "1em")
        .attr("text-anchor", "middle")
        .attr('font-size', 12+'px')
        .attr('fill', 'gray')
        .text("Host");
}