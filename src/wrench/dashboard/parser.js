const fs = require('fs');
const opn = require('opn') 
function parseFile(path) {
    return new Promise(function(resolve, reject) {
        fs.readFile(path, 'utf8', function(err, contents) {
            if (err) {
                reject(err);
            } else {
                fs.stat(path, function(err, stats) {
                    if (err) {
                        reject(err)
                    } else {
                        resolve({
                            "modified": stats.mtime,
                            "file": path,
                            "contents": JSON.parse(contents)
                        })
                    }
                })
            }
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

if (process.argv.length == 2) {
    console.log("Please provide the file name that holds the data as a command line argument")
} else {
    parseFile(process.argv[2])
        .then(function(content) {
            addToHTMLFile(content);
            opn('index.html')
            process.exit()
        })
        .catch(function(err) {
            console.log(err);
        });
}
