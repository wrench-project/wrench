var fs = require('fs');
function parseFile(path) {
    fs.readFile(path, 'utf8', function(err, contents) {
        if (err) {
            return {err}
        } else {
            return JSON.parse(contents)
        }
    });
}

    // return new Promise(function(resolve, reject) {
    //     fs.readFile(path, 'utf8', function(err, contents) {
    //         if (err) {
    //             reject(err);
    //         } else {
    //             resolve(JSON.parse(contents));
    //         }
    //     });
    // });

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


var result = {}
function generateData(filename) {
    parseFile(filename)
    .then(function(content) {
        // console.log(content)
        console.log(typeof(content))
        // return {
        //     content
        // }
        result.content = content;
        return result;
        // console.log(result)
    })
    .catch(function(err) {
        console.log(err)
        result.err = err;
        return result;
    });
}

module.exports = {
    generateData,
    parseFile
}