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

function addToHTMLFile(data, energyData) {
    var fileContents = fs.readFileSync("public/scripts.js"); //read existing contents into data
    fileContents = fileContents.toString().split('\n').slice(2).join('\n'); //remove first two lines
    fileContents = Buffer.from(fileContents, 'utf8');
    var fd = fs.openSync("public/scripts.js", 'w+');
    // var buffer = new Buffer("var data=" + JSON.stringify(data) + "\n");
    var buffer = Buffer.from("var data=" + JSON.stringify(data) + "\n" + "var energyData=" + JSON.stringify(energyData) + "\n", 'utf8')

    fs.writeSync(fd, buffer, 0, buffer.length, 0); //write new data
    fs.writeSync(fd, fileContents, 0, fileContents.length, buffer.length); //append old data
    // or fs.appendFile(fd, data);
    fs.closeSync(fd); 
}

var numProgramArguments = process.argv.length-2;
var energyData = null;

if (numProgramArguments == 0) {
    console.log("Please provide the file name that holds the data as a command line argument")
} 
else if (numProgramArguments == 2) {
   
    // Add task data from file to index.html
    // var content = parseFile(process.argv[2])
    parseFile(process.argv[2])
        .then(function(content) {
            opn('index.html')
            process.exit()

                energyFilePath = process.argv[3];
                energyData = JSON.parse(fs.readFileSync(energyFilePath));
        
                addToHTMLFile(content, energyData);
        })
        .catch(function(err) {
                console.log(err);
            });
                
    
}
else {  // Wrong number of arguments included
    console.log("Please provide the only file name/path that holds the task data as a command line argument")
}
