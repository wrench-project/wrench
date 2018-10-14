var fs = require('fs');
function parseFile(path) {
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

function addToHTMLFile(data) {
    var fileContents = fs.readFileSync("scripts.js"); //read existing contents into data
    fileContents = fileContents.toString().split('\n').slice(1).join('\n'); //remove first line
    fileContents = Buffer.from(fileContents, 'utf8');
    var fd = fs.openSync("scripts.js", 'w+');
    var buffer = new Buffer("var data=" + JSON.stringify(data) + "\n");

    fs.writeSync(fd, buffer, 0, buffer.length, 0); //write new data
    fs.writeSync(fd, fileContents, 0, fileContents.length, buffer.length); //append old data
    // or fs.appendFile(fd, data);
    fs.close(fd); 
}

parseFile("gautam.json")
    .then(function(content) {
        // prepareForGraph(content);
        // console.log(content);
        addToHTMLFile(content);
    })
    .catch(function(err) {
        console.log(err);
    });