document.addEventListener('DOMContentLoaded', function () {
    fetchConfigsAndPopulateDropdown();
});


async function fetchConfigsAndPopulateDropdown() {
    try {
        const configs = await fetchConfigs();
        populateConfigDropdown(configs);
        if (configs.length > 0) {
            fetchAndUpdateData(configs[0]);
        }
    } catch (error) {
        console.error("Error fetching configs:", error);
    }
}

async function fetchConfigs() {
    const response = await fetch('/get_configs');
    if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
    }
    return await response.json();
}

function populateConfigDropdown(configs) {
    const dropdown = document.getElementById('config-dropdown');
    dropdown.innerHTML = '';
    configs.forEach(config => {
        dropdown.innerHTML += `<option value="${config}">${config}</option>`;
    });

    dropdown.addEventListener('change', function () {
        fetchAndUpdateData(this.value);
    });
}

async function fetchAndUpdateData(configName) {
    console.log("Fetching data for config:", configName);
    try {
        const results = await fetchDataForConfig(configName);
        console.log("Fetched data:", results);  // <-- new line here
        updateTable(results);
        updateCharts(configName);
    } catch (error) {
        console.error("Error fetching data for config:", configName, error);
    }

    // Fetch the chart data and create the charts
    console.log("Fetching data for run number:", runNumber);
    try {
        const chartData = await fetch(`/get_usage_chart_data/${runNumber}`).then(response => response.json());
        createCharts(chartData);
    } catch (error) {
        console.error("Error fetching data for run number:", runNumber, error);
    }
}

async function fetchAndUpdateData_details(data) {

    // Convert the data from the server into a format that Plotly can understand
    let cpuUsageData = {
        x: [],  // timestamps
        y: [],  // cpu_usage values
        mode: 'lines+markers',
        name: 'CPU Usage'
    };

    let memoryUsageData = {
        x: [],  // timestamps
        y: [],  // memory_usage values
        mode: 'lines+markers',
        name: 'Memory Usage'
    };

    data.forEach(item => {
        cpuUsageData.x.push(item.timestamp);
        cpuUsageData.y.push(item.cpu_usage);
        memoryUsageData.x.push(item.timestamp);
        memoryUsageData.y.push(item.memory_usage);
    });

    // Create the CPU usage chart
    let cpuUsageChart = document.getElementById('cpuUsageChart');
    Plotly.newPlot(cpuUsageChart, [cpuUsageData]);

    // Create the memory usage chart
    let memoryUsageChart = document.getElementById('memoryUsageChart');
    Plotly.newPlot(memoryUsageChart, [memoryUsageData]);
}


function createCharts(chartData) {
    // Create the CPU usage chart
    let cpuUsageChart = document.getElementById('cpuUsageChart');
    Plotly.newPlot(cpuUsageChart, chartData.cpu_usage_chart.data, chartData.cpu_usage_chart.layout);

    // Create the memory usage chart
    let memoryUsageChart = document.getElementById('memoryUsageChart');
    Plotly.newPlot(memoryUsageChart, chartData.memory_usage_chart.data, chartData.memory_usage_chart.layout);
}


async function fetchDataForConfig(configName) {
    const response = await fetch(`/get_data?config_name=${configName}`);
    if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
    }
    const data = await response.text();
    return JSON.parse(JSON.parse(data));
}


function updateTable(results) {
    console.log("Type of results:", typeof results);  // <-- new line here

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
        // Add a new cell with a link to the run details page
        row.append(`<td class="border px-4 py-2"><a href="/run_details/${result.run_number}">Details</a></td>`);
        table.append(row);
    });
    createCharts(results);


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