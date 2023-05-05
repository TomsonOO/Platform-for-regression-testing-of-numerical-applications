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
            updateCharts(config_name); // Rename the function to updateCharts
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

function updateCharts(config_name) {
  fetch(`/get_all_chart_data?config_name=${config_name}`)
    .then((response) => response.json())
    .then((allChartData) => {
      // Update execution time chart
      let execution_time_chart = document.getElementById("execution_time_chart");
      Plotly.newPlot(execution_time_chart, allChartData.execution_time_chart.data, allChartData.execution_time_chart.layout);

      // Update CPU percentage chart
      let cpu_percent_chart = document.getElementById("cpu_percent_chart");
      Plotly.newPlot(cpu_percent_chart, allChartData.cpu_percent_chart.data, allChartData.cpu_percent_chart.layout);

      // Update memory percentage chart
      let memory_percent_chart = document.getElementById("memory_percent_chart");
      Plotly.newPlot(memory_percent_chart, allChartData.memory_percent_chart.data, allChartData.memory_percent_chart.layout);
    });
}