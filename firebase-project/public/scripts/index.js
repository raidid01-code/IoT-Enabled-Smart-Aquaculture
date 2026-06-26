import { app, auth, signInWithEmailAndPassword, signOut, onAuthStateChanged } from './auth.js';
import { getDatabase, ref, onValue, query, limitToLast } from "https://www.gstatic.com/firebasejs/10.8.0/firebase-database.js";
import { updateGauge } from './gauges-definition.js';
import { initCombinedChart, initTempChart, initPhChart, initTdsChart, updateChartsData } from './charts-definition.js';

const db = getDatabase(app);
let combinedChart, tempChart, phChart, tdsChart;

// Global variable to mirror the ESP32's hysteresis pump state
let uiPumpState = false; 

document.addEventListener('DOMContentLoaded', () => {
    // Initialize all charts
    combinedChart = initCombinedChart('combinedChart');
    tempChart = initTempChart('tempChart');
    phChart = initPhChart('phChart');
    tdsChart = initTdsChart('tdsChart');

    const errorText = document.getElementById('login-error');

    // Handle Login
    document.getElementById('login-form').addEventListener('submit', (e) => {
        e.preventDefault();
        const email = document.getElementById('email').value;
        const pass = document.getElementById('password').value;
        
        signInWithEmailAndPassword(auth, email, pass).catch((error) => {
            errorText.innerText = "Login Failed: " + error.message;
            errorText.style.display = "block";
        });
    });

    // Clear error message when user tries typing again
    document.getElementById('password').addEventListener('input', () => {
        errorText.style.display = "none";
    });

    // Handle Logout
    document.getElementById('logout-btn').addEventListener('click', () => {
        signOut(auth);
    });
});

onAuthStateChanged(auth, (user) => {
    if (user) {
        // Show Dashboard, Hide Login
        document.getElementById('login-view').style.display = 'none';
        document.getElementById('dashboard-view').style.display = 'block';

        const uid = user.uid;
        
        // Fetch Last 30 Readings
        const readingsRef = query(ref(db, `UsersData/${uid}/readings`), limitToLast(30));
        onValue(readingsRef, (snapshot) => {
            if (snapshot.exists()) {
                const dataObj = snapshot.val();
                const keys = Object.keys(dataObj).sort();

                const labels = [], temps = [], phs = [], tdsList = [];
                const tbody = document.getElementById('table-body');
                tbody.innerHTML = ''; 

                // Populate Table (Reverse order so newest is at the top)
                const tableKeys = [...keys].reverse();
                tableKeys.forEach(key => {
                    const reading = dataObj[key];
                    const date = new Date(reading.timestamp * 1000);
                    const timeStr = date.toLocaleTimeString('en-US', {hour12: false});

                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                        <td>${timeStr}</td>
                        <td>${reading.temperature.toFixed(2)}</td>
                        <td>${reading.ph.toFixed(2)}</td>
                        <td>${reading.tds.toFixed(2)}</td>
                    `;
                    tbody.appendChild(tr);
                });

                // Populate Chart Arrays (Chronological order)
                keys.forEach(key => {
                    const reading = dataObj[key];
                    const date = new Date(reading.timestamp * 1000);
                    labels.push(date.toLocaleTimeString('en-US', {hour12: false}));
                    temps.push(reading.temperature);
                    phs.push(reading.ph);
                    tdsList.push(reading.tds);
                });

                const latestData = dataObj[keys[keys.length - 1]];
                updateDashboardUI(latestData);
                updateChartsData(combinedChart, tempChart, phChart, tdsChart, labels, temps, phs, tdsList);
            }
        });

    } else {
        // Hide Dashboard, Show Login
        document.getElementById('login-view').style.display = 'flex';
        document.getElementById('dashboard-view').style.display = 'none';
    }
});

function updateDashboardUI(data) {
    // Update Text
    document.getElementById('temp-val').innerText = data.temperature.toFixed(1);
    document.getElementById('ph-val').innerText = data.ph.toFixed(2);
    document.getElementById('tds-val').innerText = data.tds.toFixed(1);

    // Update Gauges
    updateGauge('temp', data.temperature);
    updateGauge('ph', data.ph);
    updateGauge('tds', data.tds);

    // Replicate ESP32 Logic for UI Indicators
    const isToxic = (data.ph < 6.5 || data.ph > 9.0) && (data.temperature > 32.0);
    const alarm = (data.ph < 6.5 || data.ph > 9.0 || data.temperature < 20.0 || data.temperature > 35.0);
    
    // Exact ESP32 Pump Hysteresis Logic
    if (data.tds > 500 || isToxic) {
        uiPumpState = true;
    } else if (data.tds < 450 && !isToxic) {
        uiPumpState = false;
    }

    // Status Panel
    const statusBtn = document.getElementById('overall-status');
    if (isToxic || data.tds > 500 || alarm) {
        statusBtn.innerHTML = '<span class="status-dot-white"></span> WARNING';
        statusBtn.className = "status-btn toxic";
    } else {
        statusBtn.innerHTML = '<span class="status-dot-white"></span> NORMAL';
        statusBtn.className = "status-btn normal";
    }

    // Hardware Indicators
    const pumpInd = document.getElementById('pump-indicator');
    pumpInd.innerText = `Pump: ${uiPumpState ? 'ON' : 'OFF'}`;
    pumpInd.className = `hw-indicator ${uiPumpState ? 'on' : 'off'}`;

    const buzzerInd = document.getElementById('buzzer-indicator');
    buzzerInd.innerText = `Buzzer: ${alarm ? 'ON' : 'OFF'}`;
    buzzerInd.className = `hw-indicator ${alarm ? 'on' : 'off'}`;
}