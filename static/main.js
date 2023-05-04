let socket = io();

socket.on('connect', () => {
    console.log('Connected to the server');
    socket.emit('get_configs');
});

socket.on('configs', (configs) => {
    console.log('Received configs:', configs);
    let dropdown = $('#config-dropdown');
    dropdown.empty();
    configs.forEach(config => {
        dropdown.append(`<option value="${config}">${config}</option>`);
    });
    fetch_and_update_data(dropdown.val());
});

socket.on('update_data', (config_name) => {
    let selected_config = $('#config-dropdown').val();
    if (config_name === selected_config) {
        fetch_and_update_data(selected_config);
    }
});

$("#config-dropdown").on("change", function () {
    let selected_config = $(this).val();
    fetch_and_update_data(selected_config);
});

function fetch_and_update_data(config_name) {
    $.ajax({
        url: `/get_data?config_name=${config_name}`,
        method: 'GET',
        success: (data) => {
            let results = JSON.parse(data);
            update_table(results);
        },
        error: () => {
            console.error('Error fetching data for config:', config_name);
        }
    });
}

function update_table(results) {
    let table = $('#results-table');
    table.empty();

    // Create table headers
    let headers = Object.keys(results[0]);
    let headerRow = $('<tr>');
    headers.forEach(header => {
        headerRow.append(`<th class="border px-4 py-2">${header}</th>`);
    });
    table.append(headerRow);

    // Create table rows
    results.forEach(result => {
        let row = $('<tr>');
        headers.forEach(header => {
            row.append(`<td class="border px-4 py-2">${result[header]}</td>`);
        });
        table.append(row);
    });
}
