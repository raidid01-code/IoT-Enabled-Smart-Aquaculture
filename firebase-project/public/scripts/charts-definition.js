// Combined Chart Setup
export function initCombinedChart(canvasId) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: 'Temperature (°C)', borderColor: '#f5a623', backgroundColor: 'transparent', data: [], yAxisID: 'y', tension: 0.4 },
                { label: 'pH Level', borderColor: '#0b8a5c', backgroundColor: 'transparent', data: [], yAxisID: 'y1', tension: 0.4 },
                { label: 'TDS (ppm)', borderColor: '#34a853', backgroundColor: 'transparent', data: [], yAxisID: 'y1', tension: 0.4 }
            ]
        },
        options: {
            responsive: true, maintainAspectRatio: false,
            scales: {
                x: { display: true, title: { display: true, text: 'Time' }, grid: { display: false } },
                y: { type: 'linear', display: true, position: 'left', title: { display: true, text: 'Temperature (°C)' } },
                y1: { type: 'linear', display: true, position: 'right', title: { display: true, text: 'pH / TDS' }, grid: { drawOnChartArea: false } }
            },
            plugins: { legend: { position: 'top' } }
        }
    });
}

// Individual Chart Setups
export function initTempChart(canvasId) { return createSingleChart(canvasId, 'Temperature (°C)', '#f5a623'); }
export function initPhChart(canvasId) { return createSingleChart(canvasId, 'pH Level', '#0b8a5c'); }
export function initTdsChart(canvasId) { return createSingleChart(canvasId, 'TDS (ppm)', '#34a853'); }

function createSingleChart(canvasId, label, color) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{ label: label, borderColor: color, backgroundColor: 'rgba(0,0,0,0.05)', fill: true, data: [], tension: 0.4 }]
        },
        options: {
            responsive: true, maintainAspectRatio: false,
            scales: { x: { grid: { display: false } } }
        }
    });
}

// Global Update Function
export function updateChartsData(combined, tempC, phC, tdsC, labels, temps, phs, tds) {
    // Update Combined
    combined.data.labels = labels;
    combined.data.datasets[0].data = temps;
    combined.data.datasets[1].data = phs;
    combined.data.datasets[2].data = tds;
    combined.update('none');

    // Update Individuals
    tempC.data.labels = labels; tempC.data.datasets[0].data = temps; tempC.update('none');
    phC.data.labels = labels; phC.data.datasets[0].data = phs; phC.update('none');
    tdsC.data.labels = labels; tdsC.data.datasets[0].data = tds; tdsC.update('none');
}