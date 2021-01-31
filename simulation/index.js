const express = require('express')
const app = express()
const fs = require('fs')
const http = require('http')
const WebSocket = require('ws')

const ip = '127.0.0.1';
const port = 4200;
const wsport = 4201;
const jsonport = 4202;
const files_dir = 'files/histograms/';
const cycle_time = 10000;

const server = http.createServer(app);

// Web socket to handle serving of histogram images for cluster depths
const wss = new WebSocket.Server({
    port: wsport,
});

// Web socket to handle serving of logs for read-write operations by qemu
const json_wss = new WebSocket.Server({
    port: jsonport,
});

// List of image collection sockets
var sockets = [];

// List of log collection sockets
var json_sockets = [];

// Set folder containing static files for server
app.use(express.static('files'))

var updateImages = function() {
    // Be sure there exists at least one connected server before trying to send image links
    if (sockets.length < 1) {
        console.log("No sockets awaiting cluster depth logs....");
        setTimeout(updateImages, cycle_time);
        return;
    }

    // Read list of files and compute array to forward to display
    fs.readdir(files_dir, (err, files) => {
        if (err != null) {
            console.log("error reading files");
            setTimeout(updateImages, cycle_time);
            return;
        }
        data = [];
        current = null;
        files.forEach((file, i) => {
            // Read file info synchronously in order to have coherent loop
            item = fs.statSync(files_dir + file)
            item.name = file
            item.label = item.ctime

            // Verify if file is the latest file created. This should be the most up to date on cluster depths
            if (current == null || (current != null && current.ctimeMs < item.ctimeMs)) current = item

            data.push(item);
        });
        // Sort files by creation date
        data.sort((a, b) => {
            if (a.ctimeMs < b.ctimeMs) return 1;
            return -1;
        })
        resp = {
            "current": current,
            "older": data
        }

        sockets.forEach((ws, i) => {
            ws.send(JSON.stringify(resp));
        });

        // Set next execution time
        setTimeout(updateImages, cycle_time);
    });
}

// Load read-write logs and forward to display
var updateTables = function() {
    // Be sure at least one socket is connected before attempting to send logs
    if (json_sockets.length < 1) {
        console.log("No sockets awaiting read write logs....");
        setTimeout(updateTables, cycle_time);
        return;
    }


    read_log = [];
    if (fs.existsSync('files/json/read_log.json')) {
        read_log = fs.readFileSync('files/json/read_log.json', 'utf8');
    }

    write_log = [];
    if (fs.existsSync('files/json/write_log.json')) {
        write_log = fs.readFileSync('files/json/write_log.json', 'utf8');
    }

    resp = {
        'read_log': JSON.parse(read_log), // Read data is string so needs parsing into JSON before pushing to view
        'write_log': JSON.parse(write_log)
    }

    resp = JSON.stringify(resp);

    json_sockets.forEach((ws, i) => {
        ws.send(resp)
    });

    // Set next execution time
    setTimeout(updateTables, cycle_time);
}

wss.on('connection', (ws) => {
    sockets.push(ws);
});

wss.on('close', (ws) => {
    sockets.remove(ws);
});

json_wss.on('connection', (ws) => {
    json_sockets.push(ws);
});

json_wss.on('close', (ws) => {
    json_sockets.remove(ws);
});

app.get('/', (req, res) => {
    res.writeHead(200, { 'content-type': 'text/html' });
    fs.createReadStream('views/index.html').pipe(res);
});

server.listen(port, () => {
    console.log("Server started at " + ip + ":" + port);

    // Start information sending functions
    setTimeout(updateImages, cycle_time);
    setTimeout(updateTables, cycle_time);
})