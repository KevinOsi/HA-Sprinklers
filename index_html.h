#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Irrigation Control</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f4; }
        .container { max-width: 800px; margin: 20px auto; padding: 20px; background: white; box-shadow: 0 0 10px rgba(0,0,0,0.1); border-radius: 8px; }
        h1 { text-align: center; color: #333; }
        .card { border: 1px solid #ddd; padding: 15px; margin-bottom: 20px; border-radius: 4px; background: #fafafa; }
        .card h2 { margin-top: 0; color: #555; }
        .zone-control { display: flex; align-items: center; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #eee; }
        .zone-control:last-child { border-bottom: none; }
        .zone-name { font-weight: bold; }
        .status-indicator { display: inline-block; width: 12px; height: 12px; border-radius: 50%; margin-right: 10px; background-color: #ccc; }
        .status-on { background-color: #4CAF50; }
        .status-off { background-color: #F44336; }
        button { padding: 8px 16px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; transition: background 0.3s; }
        .btn-on { background-color: #4CAF50; color: white; }
        .btn-off { background-color: #F44336; color: white; }
        .btn-on:hover { background-color: #45a049; }
        .btn-off:hover { background-color: #d32f2f; }
        #queue-list { list-style: none; padding: 0; }
        #queue-list li { background: #eee; margin: 5px 0; padding: 8px; border-radius: 4px; font-size: 0.9em; }
        .info-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; }
        .info-item { background: #eee; padding: 10px; border-radius: 4px; font-size: 0.9em; }
    </style>
</head>
<body>

<div class="container">
    <h1>Irrigation System</h1>

    <div class="card">
        <h2>System Status</h2>
        <div class="info-grid" id="system-info">
            <div class="info-item">Loading...</div>
        </div>
    </div>

    <div class="card">
        <h2>Zone Control</h2>
        <div id="zones-container">
            <!-- Zones will be populated here -->
        </div>
    </div>

    <div class="card">
        <h2>Action Queue</h2>
        <ul id="queue-list">
            <li>Loading...</li>
        </ul>
    </div>
</div>

<script>
    const zoneNames = ["Zone 1", "Zone 2", "Zone 3", "Zone 4"];

    function fetchStatus() {
        fetch('/status')
            .then(response => response.json())
            .then(data => {
                updateSystemInfo(data.system);
                updateZones(data.relays);
                updateQueue(data.queue);
            })
            .catch(error => console.error('Error fetching status:', error));
    }

    function updateSystemInfo(system) {
        const container = document.getElementById('system-info');
        if (!system) return;

        let html = `
            <div class="info-item"><strong>Device ID:</strong> ${system.deviceID}</div>
            <div class="info-item"><strong>IP Address:</strong> ${system.ip}</div>
            <div class="info-item"><strong>Time:</strong> ${system.time}</div>
            <div class="info-item"><strong>WiFi Signal:</strong> ${system.rssi} dBm</div>
        `;
        container.innerHTML = html;
    }

    function updateZones(relays) {
        const container = document.getElementById('zones-container');
        if (!relays) return;

        let html = '';
        relays.forEach((state, index) => {
            const isOn = state === 1;
            const statusClass = isOn ? 'status-on' : 'status-off';
            const btnClass = isOn ? 'btn-off' : 'btn-on';
            const btnText = isOn ? 'Turn OFF' : 'Turn ON';
            const action = isOn ? 0 : 1;

            html += `
                <div class="zone-control">
                    <div>
                        <span class="status-indicator ${statusClass}"></span>
                        <span class="zone-name">${zoneNames[index]}</span>
                    </div>
                    <button class="${btnClass}" onclick="controlZone(${index + 1}, ${action})">${btnText}</button>
                </div>
            `;
        });
        container.innerHTML = html;
    }

    function updateQueue(queueData) {
        const list = document.getElementById('queue-list');
        if (!queueData || !queueData.Queue || queueData.Queue.length === 0) {
            list.innerHTML = '<li>No pending actions.</li>';
            return;
        }

        let html = '';
        queueData.Queue.forEach(item => {
            const date = new Date(item.T * 1000); // Epoch to JS Date
            const action = item.A === 1 ? "ON" : "OFF";
            html += `<li><strong>${date.toLocaleString()}</strong>: Set Zone ${item.R} to ${action}</li>`;
        });
        list.innerHTML = html;
    }

    function controlZone(relay, action) {
        fetch('/control', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ relay: relay, action: action }),
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                fetchStatus(); // Refresh immediately
            } else {
                alert('Command failed');
            }
        })
        .catch(error => console.error('Error sending control:', error));
    }

    // Refresh status every 2 seconds
    setInterval(fetchStatus, 2000);
    // Initial fetch
    fetchStatus();

</script>

</body>
</html>
)rawliteral";

#endif // INDEX_HTML_H
