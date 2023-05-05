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

    let selected_config = dropdown.val();
    if (selected_config) {
        fetch_and_update_data(selected_config);
    }
});

socket.on('update_data', (config_name) => {
    let selected_config = $('#config-dropdown').val();
    if (config_name === selected_config) {
        fetch_and_update_data(selected_config);
    }
});

$(document).ready(function () {
    $("#config-dropdown").change(function () {
        let selected_config = $(this).val();
        console.log("Config changed to:", selected_config);
        if (selected_config) {
            fetch_and_update_data(selected_config);
        }
    });
});

function fetch_and_update_data(config_name) {
    console.log("Fetching data for config:", config_name);
    $.ajax({
        url: `/get_data?config_name=${config_name}`,
        method: "GET",
        success: (data) => {
            let results = JSON.parse(data);
            update_table(results);
            updateChart(config_name); // Add this line to update the chart
        },
        error: () => {
            console.error("Error fetching data for config:", config_name);
        },
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

function updateChart(config_name) {
  fetch(`/get_chart_data?config_name=${config_name}`)
    .then((response) => response.json())
    .then((chartData) => {
      let chart = document.getElementById("execution_time_chart");
      Plotly.newPlot(chart, chartData.data, chartData.layout); // Include the layout here
    });
}