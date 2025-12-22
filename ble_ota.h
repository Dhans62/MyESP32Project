#ifndef WEB_H
#define WEB_H

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Honda K-Line Scanner v12.0</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
      color: #fff;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
      background: rgba(255,255,255,0.98);
      border-radius: 20px;
      padding: 30px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      color: #333;
    }
    header {
      text-align: center;
      margin-bottom: 30px;
      padding-bottom: 20px;
      border-bottom: 3px solid #667eea;
    }
    h1 {
      color: #667eea;
      font-size: 32px;
      margin-bottom: 5px;
    }
    .subtitle {
      color: #888;
      font-size: 14px;
    }
    .status-bar {
      display: flex;
      justify-content: space-between;
      background: #f8f9fa;
      padding: 15px
      border-radius: 10px;
      margin-bottom: 20px;
      border-left: 4px solid #667eea;
      flex-wrap: wrap;
      gap: 10px;
    }
    .status-item {
      display: flex;
      flex-direction: column;
      gap: 3px;
    }
    .status-label {
      font-size: 11px;
      color: #888;
      text-transform: uppercase;
      font-weight: 600;
    }
    .status-value {
      font-size: 14px;
      font-weight: bold;
      color: #667eea;
    }
    .status-value.online { color: #28a745; }
    .status-value.offline { color: #dc3545; }
    
    /* Live Data Panel */
    .live-data {
      background: linear-gradient(135deg, #1e1e1e 0%, #2d2d2d 100%);
      color: #0f0;
      padding: 25px;
      border-radius: 15px;
      margin-bottom: 20px;
      font-family: 'Courier New', monospace;
      border: 3px solid #667eea;
      box-shadow: 0 0 30px rgba(102,126,234,0.3);
      display: none;
    }
    .live-data.active { display: block; }
    
    .data-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 20px;
      margin-bottom: 15px;
    }
    .data-card {
      background: rgba(0,255,0,0.05);
      border: 2px solid rgba(0,255,0,0.2);
      border-radius: 10px;
      padding: 15px;
      text-align: center;
      transition: all 0.3s;
    }
    .data-card:hover {
      transform: translateY(-3px);
      box-shadow: 0 5px 20px rgba(0,255,0,0.3);
      border-color: #0f0;
    }
    .data-label {
      color: #888;
      font-size: 11px;
      text-transform: uppercase;
      margin-bottom: 8px;
      letter-spacing: 1px;
    }
    .data-value {
      color: #0f0;
      font-size: 28px;
      font-weight: bold;
      text-shadow: 0 0 10px rgba(0,255,0,0.5);
    }
    .data-unit {
      font-size: 14px;
      color: #888;
      margin-left: 5px;
    }
    
    /* Stats Bar */
    .stats-bar {
      background: rgba(0,255,0,0.05);
      border-top: 2px solid rgba(0,255,0,0.2);
      padding: 10px;
      border-radius: 0 0 10px 10px;
      display: flex;
      justify-content: space-around;
      font-size: 11px;
    }
    .stat-item {
      text-align: center;
    }
    .stat-value {
      color: #0f0;
      font-weight: bold;
      font-size: 13px;
    }
    
    /* Buttons */
    .button-group {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 15px;
      margin-bottom: 20px;
    }
    button {
      padding: 15px;
      font-size: 15px;
      font-weight: bold;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      transition: all 0.3s;
      box-shadow: 0 4px 10px rgba(0,0,0,0.1);
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0,0,0,0.2);
    }
    button:active {
      transform: translateY(0);
    }
    button:disabled {
      background: #ccc !important;
      cursor: not-allowed;
      transform: none !important;
    }
    
    .btn-primary {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    .btn-success {
      background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
      color: white;
    }
    .btn-warning {
      background: linear-gradient(135deg, #ffc107 0%, #ff9800 100%);
      color: #333;
    }
    .btn-danger {
      background: linear-gradient(135deg, #dc3545 0%, #c82333 100%);
      color: white;
    }
    
    /* Logs */
    #logs {
      background: #1e1e1e;
      color: #0f0;
      padding: 20px;
      border-radius: 10px;
      font-family: 'Courier New', monospace;
      font-size: 11px;
      max-height: 400px;
      overflow-y: auto;
      white-space: pre-wrap;
      word-wrap: break-word;
      border: 2px solid #333;
      box-shadow: inset 0 0 20px rgba(0,0,0,0.5);
    }
    #logs::-webkit-scrollbar {
      width: 8px;
    }
    #logs::-webkit-scrollbar-track {
      background: #2d2d2d;
    }
    #logs::-webkit-scrollbar-thumb {
      background: #667eea;
      border-radius: 4px;
    }
    
    /* Spinner */
    .spinner {
      border: 4px solid #f3f3f3;
      border-top: 4px solid #667eea;
      border-radius: 50%;
      width: 40px;
      height: 40px;
      animation: spin 1s linear infinite;
      margin: 20px auto;
      display: none;
    }
    @keyframes spin {
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
    }
    
    /* Alerts */
    .alert {
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 15px;
      font-weight: 500;
      display: none;
    }
    .alert.show { display: block; }
    .alert-success {
      background: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }
    .alert-danger {
      background: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
    .alert-warning {
      background: #fff3cd;
      color: #856404;
      border: 1px solid #ffeaa7;
    }
    
    /* Responsive */
    @media (max-width: 600px) {
      .button-group {
        grid-template-columns: 1fr;
      }
      .data-grid {
        grid-template-columns: repeat(2, 1fr);
      }
      h1 { font-size: 24px; }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>🏍️ Honda K-Line Scanner</h1>
      <div class="subtitle">ESP32 ECU Diagnostic Tool v12.0 Ultimate Edition</div>
    </header>
    
    <!-- Alert Messages -->
    <div id="alertBox" class="alert"></div>
    
    <!-- Status Bar -->
    <div class="status-bar">
      <div class="status-item">
        <span class="status-label">Protocol</span>
        <span class="status-value" id="protocol">Not Connected</span>
      </div>
      <div class="status-item">
        <span class="status-label">Connection</span>
        <span class="status-value offline" id="connStatus">Offline</span>
      </div>
      <div class="status-item">
        <span class="status-label">Uptime</span>
        <span class="status-value" id="uptime">0s</span>
      </div>
      <div class="status-item">
        <span class="status-label">Success Rate</span>
        <span class="status-value" id="successRate">0%</span>
      </div>
    </div>
    
    <!-- Live Data Display -->
    <div class="live-data" id="liveData">
      <div class="data-grid">
        <div class="data-card">
          <div class="data-label">RPM</div>
          <div class="data-value" id="rpm">0</div>
        </div>
        <div class="data-card">
          <div class="data-label">TPS</div>
          <div class="data-value" id="tps">0<span class="data-unit">%</span></div>
        </div>
        <div class="data-card">
          <div class="data-label">Temperature</div>
          <div class="data-value" id="temp">0<span class="data-unit">°C</span></div>
        </div>
        <div class="data-card">
          <div class="data-label">Speed</div>
          <div class="data-value" id="speed">0<span class="data-unit">km/h</span></div>
        </div>
        <div class="data-card">
          <div class="data-label">Battery</div>
          <div class="data-value" id="battery">0<span class="data-unit">V</span></div>
        </div>
      </div>
      
      <div class="stats-bar">
        <div class="stat-item">
          <div class="stat-label">Total Packets</div>
          <div class="stat-value" id="totalPackets">0</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Errors</div>
          <div class="stat-value" id="errorPackets">0</div>
        </div>
        <div class="stat-item">
          <div class="stat-label">Data Rate</div>
          <div class="stat-value" id="dataRate">0 pkt/s</div>
        </div>
      </div>
    </div>
    
    <!-- Control Buttons -->
    <div class="button-group">
      <button class="btn-primary" id="scanBtn" onclick="startScan()">
        🔍 Scan Protocols
      </button>
      <button class="btn-success" onclick="refreshLogs()">
        📄 Refresh Logs
      </button>
      <button class="btn-warning" id="reconnectBtn" onclick="reconnect()" disabled>
        🔄 Reconnect
      </button>
      <button class="btn-danger" onclick="clearLogs()">
        🗑️ Clear Logs
      </button>
    </div>
    
    <div class="spinner" id="spinner"></div>
    
    <!-- Logs Display -->
    <div id="logs">Waiting for connection...</div>
  </div>

  <script>
    let dataRefresh = null;
    let isConnected = false;
    let lastPacketCount = 0;
    let lastPacketTime = Date.now();

    // Show alert message
    function showAlert(message, type) {
      const alert = document.getElementById('alertBox');
      alert.className = 'alert alert-' + type + ' show';
      alert.textContent = message;
      setTimeout(() => {
        alert.classList.remove('show');
      }, 5000);
    }

    // Start scan
    function startScan() {
      const btn = document.getElementById('scanBtn');
      const spinner = document.getElementById('spinner');
      
      btn.disabled = true;
      btn.textContent = '⏳ Scanning...';
      spinner.style.display = 'block';
      
      showAlert('Starting ECU scan...', 'warning');
      
      setTimeout(refreshLogs, 1000);
      setTimeout(refreshLogs, 3000);
      setTimeout(() => {
        btn.disabled = false;
        btn.textContent = '🔍 Scan Protocols';
        spinner.style.display = 'none';
        checkConnection();
      }, 5000);
    }

    // Manual reconnect
    function reconnect() {
      const btn = document.getElementById('reconnectBtn');
      btn.disabled = true;
      btn.textContent = '⏳ Reconnecting...';
      
      showAlert('Attempting to reconnect...', 'warning');
      
      fetch('/reconnect', {method: 'POST'})
        .then(r => r.text())
        .then(text => {
          if(text === 'OK') {
            showAlert('✓ Reconnected successfully!', 'success');
            checkConnection();
          } else {
            showAlert('✗ Reconnection failed', 'danger');
          }
          btn.disabled = false;
          btn.textContent = '🔄 Reconnect';
        })
        .catch(e => {
          showAlert('✗ Reconnection error', 'danger');
          btn.disabled = false;
          btn.textContent = '🔄 Reconnect';
        });
    }

    // Refresh live data
    function refreshData() {
      if(!isConnected) return;
      
      fetch('/data')
        .then(r => r.json())
        .then(data => {
          if(data.valid) {
            document.getElementById('rpm').textContent = data.rpm;
            document.getElementById('tps').innerHTML = data.tps + '<span class="data-unit">%</span>';
            document.getElementById('temp').innerHTML = data.temp + '<span class="data-unit">°C</span>';
            document.getElementById('speed').innerHTML = data.speed + '<span class="data-unit">km/h</span>';
            document.getElementById('battery').innerHTML = data.battery.toFixed(1) + '<span class="data-unit">V</span>';
            
            // Update stats
            document.getElementById('totalPackets').textContent = data.totalPackets;
            document.getElementById('errorPackets').textContent = data.errorPackets;
            document.getElementById('uptime').textContent = formatUptime(data.uptime);
            
            // Calculate success rate
            let successRate = 100;
            if(data.totalPackets > 0) {
              successRate = ((data.totalPackets - data.errorPackets) / data.totalPackets * 100).toFixed(1);
            }
            document.getElementById('successRate').textContent = successRate + '%';
            
            // Calculate data rate
            const now = Date.now();
            const timeDiff = (now - lastPacketTime) / 1000;
            const packetDiff = data.totalPackets - lastPacketCount;
            const rate = timeDiff > 0 ? (packetDiff / timeDiff).toFixed(1) : 0;
            document.getElementById('dataRate').textContent = rate + ' pkt/s';
            
            lastPacketCount = data.totalPackets;
            lastPacketTime = now;
          }
        })
        .catch(e => console.log('Data fetch error:', e));
    }

    // Check connection status
    function checkConnection() {
      fetch('/data')
        .then(r => r.json())
        .then(data => {
          const wasConnected = isConnected;
          isConnected = data.connected && data.valid;
          
          const statusEl = document.getElementById('connStatus');
          const reconnectBtn = document.getElementById('reconnectBtn');
          
          if(isConnected) {
            statusEl.textContent = 'Online';
            statusEl.className = 'status-value online';
            document.getElementById('liveData').classList.add('active');
            document.getElementById('protocol').innerHTML = '<span style="color: #28a745;">✓ ' + data.mode + '</span>';
            reconnectBtn.disabled = true;
            
            if(!dataRefresh) {
              dataRefresh = setInterval(refreshData, 500);
            }
            
            if(!wasConnected) {
              showAlert('✓ ECU connected successfully!', 'success');
            }
          } else {
            statusEl.textContent = 'Offline';
            statusEl.className = 'status-value offline';
            document.getElementById('liveData').classList.remove('active');
            document.getElementById('protocol').innerHTML = '<span style="color: #dc3545;">✗ Not Connected</span>';
            reconnectBtn.disabled = false;
            
            if(dataRefresh) {
              clearInterval(dataRefresh);
              dataRefresh = null;
            }
            
            if(wasConnected) {
              showAlert('⚠ Connection lost - Auto-reconnect active', 'warning');
            }
          }
        })
        .catch(e => {
          isConnected = false;
          document.getElementById('connStatus').textContent = 'Offline';
          document.getElementById('connStatus').className = 'status-value offline';
        });
    }

    // Refresh logs
    function refreshLogs() {
      fetch('/logs')
        .then(r => r.text())
        .then(text => {
          const logsDiv = document.getElementById('logs');
          logsDiv.textContent = text;
          logsDiv.scrollTop = logsDiv.scrollHeight;
        });
    }

    // Clear logs (client-side only)
    function clearLogs() {
      const logsDiv = document.getElementById('logs');
      logsDiv.textContent = 'Logs cleared (refresh to see new logs)';
      showAlert('Logs display cleared', 'success');
    }

    // Format uptime
    function formatUptime(seconds) {
      if(seconds < 60) return seconds + 's';
      if(seconds < 3600) return Math.floor(seconds / 60) + 'm';
      const hours = Math.floor(seconds / 3600);
      const mins = Math.floor((seconds % 3600) / 60);
      return hours + 'h ' + mins + 'm';
    }

    // Auto-check connection every 2s
    setInterval(checkConnection, 2000);
    
    // Initial checks
    checkConnection();
    refreshLogs();
  </script>
</body>
</html>
)rawliteral";

const char PAGE_OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>OTA Firmware Update</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 20px;
    }
    .container {
      background: white;
      padding: 50px;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      text-align: center;
      max-width: 500px;
      width: 100%;
    }
    h1 {
      color: #667eea;
      margin-bottom: 30px;
      font-size: 28px;
    }
    input[type="file"] {
      margin: 20px 0;
      padding: 15px;
      border: 3px dashed #667eea;
      border-radius: 10px;
      width: 100%;
      cursor: pointer;
      transition: all 0.3s;
    }
    input[type="file"]:hover {
      border-color: #764ba2;
      background: #f8f9fa;
    }
    button {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 15px 40px;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      font-size: 16px;
      font-weight: bold;
      transition: all 0.3s;
      box-shadow: 0 4px 15px rgba(102,126,234,0.4);
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(102,126,234,0.6);
    }
    button:active {
      transform: translateY(0);
    }
    .warning {
      background: #fff3cd;
      color: #856404;
      padding: 15px;
      border-radius: 10px;
      margin: 20px 0;
      border: 1px solid #ffeaa7;
      font-size: 14px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔄 OTA Firmware Update</h1>
    <div class="warning">
      ⚠️ Warning: Do not disconnect power during update!
    </div>
    <form method="POST" action="/update" enctype="multipart/form-data">
      <input type="file" name="update" accept=".bin" required>
      <br><br>
      <button type="submit">📤 Upload & Update</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

#endif
