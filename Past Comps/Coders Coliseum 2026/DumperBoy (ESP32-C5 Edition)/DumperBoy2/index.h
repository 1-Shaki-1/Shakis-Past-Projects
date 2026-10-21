const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DumperBoy Control Panel</title>
    <style>
        * { box-sizing: border-box; border-radius: 0px; }
        body { background-color: #0b0f19; color: #f1f5f9; font-family: monospace, sans-serif; margin: 0; padding: 10px; }
        .container { max-width: 600px; margin: 0 auto; }
        h1 { font-size: 1.4rem; text-align: center; color: #38bdf8; border-bottom: 2px solid #38bdf8; padding-bottom: 8px; margin-bottom: 15px; }
        .panel { background-color: #111827; border: 1px solid #374151; padding: 12px; margin-bottom: 12px; }
        .panel h2 { font-size: 1rem; margin: 0 0 10px 0; color: #9ca3af; text-transform: uppercase; display: flex; justify-content: space-between; align-items: center;}
        .row { display: flex; justify-content: space-between; align-items: center; margin: 8px 0; font-size: 0.95rem; }
        
        button { background-color: #1f2937; color: #38bdf8; border: 1px solid #38bdf8; padding: 10px 14px; font-family: monospace; font-weight: bold; cursor: pointer; text-transform: uppercase; width: 100%; margin-top: 6px; user-select: none; }
        button:active { opacity: 0.8; }
        .button-group { display: flex; gap: 10px; }
        
        .btn-start { background-color: #064e3b; color: #34d399; border-color: #059669; }
        .btn-stop { background-color: #7f1d1d; color: #f87171; border-color: #dc2626; }
        .btn-cal { background-color: #422006; color: #f59e0b; border-color: #d97706; }
        .btn-settings { background-color: #374151; color: #d1d5db; border-color: #4b5563; margin-bottom: 12px; }
        .btn-toggle { background-color: #1e3a8a; color: #93c5fd; border-color: #3b82f6; }
        
        .gamepad-container { display: none; margin-top: 15px; }
        .gamepad { display: grid; grid-template-columns: 70px 70px 70px; grid-template-rows: 70px 70px 70px; gap: 10px; justify-content: center; margin: 15px 0; }
        .pad-btn { border-radius: 8px !important; font-size: 1.3rem; background-color: #1f2937; margin: 0; touch-action: manipulation; display: flex; align-items: center; justify-content: center;}
        .pad-btn:active, .pad-btn.active { background-color: #38bdf8; color: #0b0f19;}
        .pad-up { grid-column: 2; grid-row: 1; }
        .pad-left { grid-column: 1; grid-row: 2; }
        .pad-right { grid-column: 3; grid-row: 2; }
        .pad-down { grid-column: 2; grid-row: 3; }
        
        .dumper-controls { display: flex; gap: 10px; margin-top: 10px; }
        input[type="range"] { width: 100%; margin: 10px 0; accent-color: #38bdf8; }
        
        .badge { padding: 2px 6px; font-size: 0.8rem; font-weight: bold; }
        .online { background-color: #064e3b; color: #34d399; border: 1px solid #059669; }
        .offline { background-color: #7f1d1d; color: #f87171; border: 1px solid #dc2626; }
        .mode-auto { background-color: #4c1d95; color: #c4b5fd; border: 1px solid #7c3aed; }
        .mode-manual { background-color: #9a3412; color: #fdba74; border: 1px solid #ea580c; }
    </style>
</head>
<body>
    <div class="container">
        <h1>DumperBoy Telemetry</h1>

        <div class="panel">
            <h2>Mode Select <span id="mode-badge" class="badge mode-auto">AUTO</span></h2>
            <button class="btn-toggle" onclick="toggleMode()">SWITCH TO MANUAL</button>
        </div>

        <div class="panel" id="auto-section">
            <h2>Autonomous Controls</h2>
            <div class="button-group">
                <button class="btn-start" onclick="sendAction('start')">START</button>
                <button class="btn-stop" onclick="sendAction('stop')">STOP</button>
                <button class="btn-cal" onclick="sendAction('calibrate')">CALIBRATE</button>
            </div>
        </div>

        <div class="panel gamepad-container" id="manual-section">
            <h2>Manual Drive (WASD / D-Pad)</h2>
            <div class="gamepad">
                <button class="pad-btn pad-up" id="btn-f">W</button>
                <button class="pad-btn pad-left" id="btn-l">A</button>
                <button class="pad-btn pad-right" id="btn-r">D</button>
                <button class="pad-btn pad-down" id="btn-b">S</button>
            </div>
            <h2>Servo Dumper</h2>
            <div class="dumper-controls">
                <button onclick="sendDumper(1)">DUMP</button>
                <button onclick="sendDumper(0)">RETRACT</button>
            </div>
        </div>

        <div class="panel">
            <h2>Live Telemetry</h2>
            <div class="row"><span>Heading Error:</span><strong id="error-display">0.00°</strong></div>
            <div class="row"><span>Wall Distance:</span><strong id="distance-display">0 cm</strong></div>
            <div class="row"><span>Wi-Fi Status:</span><span id="wifi-status" class="badge offline">DISCONNECTED</span></div>
        </div>

        <button class="btn-settings" onclick="toggleSettings()">⚙️ Toggle Settings</button>

        <div id="settings-section" style="display: none;">
            
            <div class="panel">
                <h2>Speed Tuning</h2>
                <div class="row"><span>Auto Base Speed: <span id="speed-val">180</span></span></div>
                <input type="range" id="speedSlider" min="100" max="230" value="180" oninput="document.getElementById('speed-val').innerText=this.value" onchange="sendSet('speed', this.value)">
                    
                <div class="row"><span>Auto Turn Speed: <span id="auto-turn-val">150</span></span></div>
                <input type="range" id="autoTurnSlider" min="60" max="230" value="150" oninput="document.getElementById('auto-turn-val').innerText=this.value" onchange="sendSet('turnSpeed', this.value)">

                <div class="row"><span>Manual Turn Speed: <span id="turn-speed-val">150</span></span></div>
                <input type="range" id="turnSpeedSlider" min="60" max="230" value="150" oninput="document.getElementById('turn-speed-val').innerText=this.value" onchange="sendSet('turnSpeedM', this.value)">
            </div>

            <div class="panel">
                <h2>Autonomous Parameters</h2>
                <div class="row"><span id="minDist-display">Dump Distance: 15 cm</span><button onclick="updateSetting('minDist')">Edit</button></div>
                <div class="row"><span id="turnAngle-display">Turn Angle: 90.0°</span><button onclick="updateSetting('turnAngle')">Edit</button></div>
            </div>

            <div class="panel">
                <h2>PID Adjustments</h2>
                <div class="row"><span id="kp-display">Kp: 16.00</span><button onclick="updateSetting('kp')">Edit Kp</button></div>
                <div class="row"><span id="ki-display">Ki: 0.045</span><button onclick="updateSetting('ki')">Edit Ki</button></div>
                <div class="row"><span id="kd-display">Kd: 0.45</span><button onclick="updateSetting('kd')">Edit Kd</button></div>
            </div>
        </div>
    </div>

    <script>
        let isManual = false;
        let activeDirection = null;

        function toggleMode() {
            isManual = !isManual;
            fetch('/mode?manual=' + (isManual ? '1' : '0')).catch(err => console.error(err));
            
            document.getElementById('auto-section').style.display = isManual ? 'none' : 'block';
            document.getElementById('manual-section').style.display = isManual ? 'block' : 'none';
            document.querySelector('.btn-toggle').innerText = isManual ? 'SWITCH TO AUTO' : 'SWITCH TO MANUAL';
        }

        function toggleSettings() {
            let sec = document.getElementById('settings-section');
            sec.style.display = (sec.style.display === 'none') ? 'block' : 'none';
        }

        function sendAction(action) { fetch('/' + action).catch(err => console.error(err)); }
        function sendDumper(state) { fetch('/dumper?state=' + state).catch(err => console.error(err)); }
        
        // Unified function for slider inputs
        function sendSet(param, val) { fetch('/set?' + param + '=' + val).catch(err => console.error(err)); }

        // Unified function for prompt inputs (PID, Angles, Distance)
        function updateSetting(term) {
            let val = prompt("Enter new value for " + term.toUpperCase() + ":");
            if (val !== null && val.trim() !== "" && !isNaN(val)) {
                fetch('/set?' + term + '=' + parseFloat(val)).catch(err => console.error(err));
            }
        }

        function sendMove(dir) {
            if (!isManual && dir !== 'S') return;
            if (activeDirection === dir) return;
            activeDirection = dir;
            fetch('/move?dir=' + dir).catch(err => console.error(err));
        }

        function bindPad(id, dir) {
            let btn = document.getElementById(id);
            let press = (e) => { e.preventDefault(); sendMove(dir); btn.classList.add('active'); };
            let release = (e) => { e.preventDefault(); sendMove('S'); btn.classList.remove('active'); };
            
            btn.addEventListener('mousedown', press);
            btn.addEventListener('touchstart', press);
            btn.addEventListener('mouseup', release);
            btn.addEventListener('mouseleave', release);
            btn.addEventListener('touchend', release);
        }
        
        bindPad('btn-f', 'F'); bindPad('btn-b', 'B'); bindPad('btn-l', 'L'); bindPad('btn-r', 'R');

        const keyMap = { 'w': 'F', 'W': 'F', 's': 'B', 'S': 'B', 'a': 'L', 'A': 'L', 'd': 'R', 'D': 'R' };
        const keyBtnMap = { 'F': 'btn-f', 'B': 'btn-b', 'L': 'btn-l', 'R': 'btn-r' };

        window.addEventListener('keydown', (e) => {
            if (!isManual || e.repeat) return;
            let dir = keyMap[e.key];
            if (dir) {
                sendMove(dir);
                let btn = document.getElementById(keyBtnMap[dir]);
                if (btn) btn.classList.add('active');
            }
        });

        window.addEventListener('keyup', (e) => {
            if (!isManual) return;
            let dir = keyMap[e.key];
            if (dir && activeDirection === dir) {
                sendMove('S');
                let btn = document.getElementById(keyBtnMap[dir]);
                if (btn) btn.classList.remove('active');
            }
        });

        // Telemetry loop
        setInterval(() => {
            fetch('/data')
                .then(res => res.json())
                .then(data => {
                    document.getElementById('error-display').innerText = data.error.toFixed(2) + '°';
                    document.getElementById('distance-display').innerText = data.distance + ' cm';
                    
                    // Sliders
                    document.getElementById('speed-val').innerText = data.speed;
                    if (document.activeElement !== document.getElementById('speedSlider')) {
                        document.getElementById('speedSlider').value = data.speed;
                    }
                    
                    document.getElementById('turn-speed-val').innerText = data.turnSpeedM;
                    if (document.activeElement !== document.getElementById('turnSpeedSlider')) {
                        document.getElementById('turnSpeedSlider').value = data.turnSpeedM;
                    }

                    // Prompt values
                    document.getElementById('minDist-display').innerText = "Dump Distance: " + data.minDist + " cm";
                    document.getElementById('turnAngle-display').innerText = "Turn Angle: " + data.turnAngle + "°";
                    document.getElementById('kp-display').innerText = "Kp: " + data.kp.toFixed(2);
                    document.getElementById('ki-display').innerText = "Ki: " + data.ki.toFixed(3);
                    document.getElementById('kd-display').innerText = "Kd: " + data.kd.toFixed(3);

                    let w = document.getElementById('wifi-status');
                    w.innerText = data.wifiStatus;
                    w.className = data.wifiStatus === "CONNECTED" ? "badge online" : "badge offline";
                    
                    let m = document.getElementById('mode-badge');
                    m.innerText = data.mode;
                    m.className = data.mode === "MANUAL" ? "badge mode-manual" : "badge mode-auto";
                    
                    document.getElementById('auto-turn-val').innerText = data.turnSpeed;
                    if (document.activeElement !== document.getElementById('autoTurnSlider')) {
                        document.getElementById('autoTurnSlider').value = data.turnSpeed;
                    }
                })
                .catch(err => console.error(err));
        }, 250);
    </script>
</body>
</html>
)rawliteral";