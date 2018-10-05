function parseFile(path) {
    var fs = require('fs');
    return new Promise(function(resolve, reject) {
        fs.readFile(path, 'utf8', function(err, contents) {
            if (err) {
                reject(err);
            } else {
                resolve(JSON.parse(contents));
            }
        });
    });
    
}

function prepareForGraph(data) {
    var keys = ['read', 'compute', 'write', 'whole_task'];

    data.forEach(function(task) {
        task.start = task['whole_task'].start;
        keys.forEach(function(k) {
            task[k] = task[k].end - task[k].start;
        });
    });
}

parseFile("gautam.json")
    .then(function(content) {
        prepareForGraph(content);
        console.log(content);
    })
    .catch(function(err) {
        console.log(err);
    });