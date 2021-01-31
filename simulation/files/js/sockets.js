const socket_url = "127.0.0.1:4201";
const log_socket_url = "127.0.0.1:4202";
var dataSocket = null;
var logSocket = null;

// Initialize data tables
let read_table = $('#read-table').DataTable({
    columns: [
        { data: 'cluster' },
        { data: 'durée' },
        { data: 'cur_bytes' },
        { data: 'bytes' },
        { data: 'nb_occurence' },
        { data: 'read_intensity(ops)' },
        { data: 'débit(o/s)' }
    ]
});

let write_table = $('#write-table').DataTable({
    columns: [
        { data: 'cluster' },
        { data: 'durée' },
        { data: 'cur_bytes' },
        { data: 'bytes' },
        { data: 'nb_occurence' },
        { data: 'read_intensity(ops)' },
        { data: 'débit(o/s)' }
    ]
});

$(document).ready(function() {
    createSocket();
    createLogSocket();
});

function createSocket() {
    dataSocket = new WebSocket('ws://' + socket_url);

    dataSocket.onmessage = function(messageEvent) {
        let data = JSON.parse(messageEvent.data);
        let images = data.older;
        let html = "";
        for (item of images) {
            html += '<div class="col-3">' +
                '<img src="/histograms/' + item.name + '" alt="' + item.label + '">' +
                '<span class="font-weight-bold">' + item.label + '</span>' +
                '</div>';
        }
        $('#old-images').html(html);

        html = '<img src="/histograms/' + data.current.name + '" alt="' + data.current.label + '"><br>' +
            '<span class="font-weight-bold">' + data.current.label + '</span>';
        $('#latest').html(html);
    }
}

function createLogSocket() {
    logSocket = new WebSocket('ws://' + log_socket_url);

    logSocket.onmessage = function(messageEvent) {
        let data = JSON.parse(messageEvent.data);
        console.log(data);
        read_table.clear();
        write_table.clear();

        read_table.rows.add(data.read_log);
        read_table.draw();

        write_table.rows.add(data.write_log);
        write_table.draw();
    }
}

function closeSockets() {
    dataSocket.close();
    logSocket.close();
}