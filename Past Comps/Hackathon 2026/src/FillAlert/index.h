const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Trash Can Dashboard</title>
    <style>
        body {
            font-family: Verdana, Geneva, Tahoma, sans-serif;
            background-color: #0f172a;
            color: #ffffff;
            margin: 0;
            height: 100vh;
            width: 100vw;
            overflow-x: hidden;
            overflow-y: auto;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: flex-start;
            padding: 1.5vh 1vw;
            box-sizing: border-box;
        }

        /* Main Title */
        .main-title {
            font-size: clamp(1.8vh, 2.2vw, 3.2vh);
            font-weight: bold;
            background-color: #1e293b;
            padding: 1vh 2vw;
            border-radius: 1vw;
            border: 0.25vw solid #334155;
            text-align: center;
            margin-bottom: 0.8vh;
            box-shadow: 0 0.3vh 0.6vh rgba(0, 0, 0, 0.3);
            letter-spacing: -0.03vw;
        }

        /* Subtitle Banner */
        .subtitle {
            font-size: clamp(1.2vh, 1.1vw, 2vh);
            background-color: #1e293b;
            padding: 0.6vh 1.8vw;
            border-radius: 0.8vw;
            border: 0.25vw solid #334155;
            text-align: center;
            margin-bottom: 1.5vh;
            color: #f8fafc;
        }

        /* Dashboard Layout */
        .dashboard-layout {
            display: flex;
            gap: 1.5vw;
            width: 90vw;
            max-width: 1400px;
            justify-content: center;
            align-items: stretch;
        }

        /* Left Column Cards */
        .left-column {
            display: flex;
            flex-direction: column;
            gap: 1vh;
            width: 38vw;
        }

        .card {
            background-color: #1e293b;
            border: 0.25vw solid #334155;
            border-radius: 1vw;
            padding: 1vh 1.2vw;
            font-size: clamp(1.2vh, 1.1vw, 2vh);
            display: flex;
            align-items: center;
            justify-content: space-between;
            min-height: 4.8vh;
            box-shadow: 0 0.3vh 0.6vh rgba(0, 0, 0, 0.2);
            color: #ffffff;
            box-sizing: border-box;
        }

        /* Specialized Layout for Fill Card */
        .fill-card {
            flex-direction: column;
            justify-content: center;
            align-items: stretch;
            gap: 0.5vh;
            padding: 0.8vh 1.2vw;
        }

        .fill-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            width: 100%;
        }

        /* Multi-row Card Layout */
        .multi-row-card {
            flex-direction: column;
            justify-content: center;
            align-items: stretch;
            gap: 0.6vh;
            padding: 0.8vh 1.2vw;
        }

        .card-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            width: 100%;
        }

        /* System Health Section Header */
        .section-header {
            font-size: clamp(1.1vh, 1vw, 1.6vh);
            font-weight: bold;
            color: #94a3b8;
            text-transform: uppercase;
            letter-spacing: 0.05vw;
            border-bottom: 0.15vh solid #334155;
            padding-bottom: 0.3vh;
            margin-bottom: 0.2vh;
        }

        /* Combined Height & Input Card Layout */
        .height-card {
            flex-direction: column;
            justify-content: center;
            align-items: stretch;
            gap: 0.5vh;
            padding: 0.8vh 1.2vw;
        }

        .height-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            width: 100%;
        }

        .small-input {
            width: 100%;
            background-color: #0f172a;
            border: 0.15vw solid #334155;
            border-radius: 0.5vw;
            color: #ffffff;
            padding: 0.5vh 0.6vw;
            font-size: clamp(1.1vh, 0.9vw, 1.6vh);
            font-family: inherit;
            box-sizing: border-box;
            outline: none;
            transition: border-color 0.2s ease-in-out;
        }

        .small-input:focus {
            border-color: #0080ff;
        }

        /* Progress Bar Styles */
        .progress-container {
            width: 100%;
            height: 1.4vh;
            min-height: 1.2vh;
            background-color: #0f172a;
            border-radius: 1vw;
            border: 0.15vw solid #334155;
            overflow: hidden;
            box-shadow: inset 0 0.15vh 0.3vh rgba(0, 0, 0, 0.5);
        }

        .progress-bar {
            height: 100%;
            width: 0%;
            background-color: #0080ff;
            border-radius: 0.8vw;
            transition: width 0.4s ease-out, background-color 0.4s ease-out;
        }

        /* LED & Status Badge Pill */
        .badge {
            background-color: #197500;
            color: #ffffff;
            padding: 0.25vh 0.8vw;
            border-radius: 0.6vw;
            font-size: clamp(1vh, 0.9vw, 1.5vh);
            letter-spacing: 0.05vw;
            font-weight: bold;
        }

        /* Right Column (Location Card) */
        .right-column {
            width: 50vw;
            background-color: #1e293b;
            border: 0.25vw solid #334155;
            border-radius: 1.2vw;
            display: flex;
            flex-direction: column;
            overflow: hidden;
            box-shadow: 0 0.3vh 0.6vh rgba(0, 0, 0, 0.2);
            padding: 0.8vh;
            box-sizing: border-box;
            gap: 0.8vh;
        }

        .location-header {
            font-size: clamp(1.4vh, 1.4vw, 2.2vh);
            font-weight: bold;
            text-align: center;
            padding: 0.8vh;
            border: 0.25vw solid #334155;
            border-radius: 0.8vw;
            background-color: #1e293b;
        }

        .map-container {
            flex-grow: 1;
            background-color: #0f172a;
            border: 0.25vw solid #334155;
            border-radius: 0.8vw;
            display: flex;
            align-items: center;
            justify-content: center;
            overflow: hidden;
            min-height: 250px;
        }

        .map-container iframe {
            width: 100%;
            height: 100%;
            border: none;
        }

        /* Mobile Responsive Breakpoint */
        @media (max-width: 768px) {
            body {
                width: 100vw;
                height: auto;
                padding-top: 1.5vh;
            }

            .main-title {
                font-size: clamp(2vh, 4.5vw, 3.2vh);
                width: 90vw;
                border-radius: 2vw;
                border-width: 0.4vw;
                padding: 1.2vh 0;
            }

            .subtitle {
                font-size: clamp(1.4vh, 3vw, 2.2vh);
                width: 90vw;
                border-radius: 2vw;
                border-width: 0.4vw;
                padding: 0.8vh 0;
                margin-bottom: 1.5vh;
            }

            .dashboard-layout {
                flex-direction: column;
                width: 92vw;
                gap: 1.5vh;
            }

            .left-column {
                width: 100%;
                gap: 1vh;
            }

            .right-column {
                width: 100%;
                height: 40vh;
                min-height: 30vh;
                border-radius: 2vw;
                border-width: 0.4vw;
                padding: 1vh;
            }

            .card {
                width: 100%;
                font-size: clamp(1.4vh, 3.2vw, 2.2vh);
                border-radius: 2vw;
                border-width: 0.4vw;
                padding: 1.2vh 3vw;
                min-height: 5vh;
            }

            .small-input {
                font-size: clamp(1.3vh, 3vw, 2vh);
                padding: 0.8vh 1.5vw;
                border-radius: 1.5vw;
                border-width: 0.3vw;
            }

            .badge {
                font-size: clamp(1.2vh, 2.8vw, 1.8vh);
                padding: 0.4vh 2vw;
                border-radius: 1.5vw;
            }

            .section-header {
                font-size: clamp(1.3vh, 3vw, 2vh);
                border-bottom-width: 0.2vh;
            }

            .location-header {
                font-size: clamp(1.5vh, 3.5vw, 2.4vh);
                border-radius: 1.5vw;
                border-width: 0.4vw;
                padding: 0.8vh;
            }

            .map-container {
                border-radius: 1.5vw;
                border-width: 0.4vw;
                min-height: 20vh;
            }

            .progress-container {
                height: 1.8vh;
                border-radius: 1.5vw;
                border-width: 0.25vw;
            }
        }
    </style>
</head>

<body>

    <div class="main-title">Smart Trash Can Dashboard</div>
    <div class="subtitle">By the makers of FillAlert</div>

    <div class="dashboard-layout">
        <!-- Left Column -->
        <div class="left-column">
            <!-- Fill Level -->
            <div class="card fill-card">
                <div class="fill-header">
                    Fill Level
                    <span id="percentage">0%</span>
                </div>
                <div class="progress-container">
                    <div class="progress-bar" id="progress-bar"></div>
                </div>
            </div>

            <!-- Sensor Distance -->
            <div class="card">
                Sensor Distance: <span id="distance">0 cm</span>
            </div>

            <!-- Combined Collection & LED Status Card -->
            <div class="card multi-row-card">
                <div class="card-row">
                    <span>Collection Status:</span>
                    <span class="badge" id="collection-status" style="background-color: #197500;">NORMAL</span>
                </div>
                <div class="card-row">
                    <span>LED Color:</span>
                    <span class="badge" id="led">GREEN</span>
                </div>
            </div>

            <!-- Gyroscope Tilt & Lid Angle Card -->
            <div class="card multi-row-card">
                <div class="card-row">
                    <span>Box Tilt Angle:</span>
                    <span id="gyro-angle">0.0°</span>
                </div>
                <div class="card-row">
                    <span>Lid Position:</span>
                    <span class="badge" id="lid-status-badge" style="background-color: #197500;">CLOSED</span>
                </div>
            </div>

            <!-- System Health Card -->
            <div class="card multi-row-card">
                <div class="section-header">System Health</div>
                <div class="card-row">
                    <span>ESP32:</span>
                    <span class="badge" id="esp32-status" style="background-color: #197500;">ONLINE</span>
                </div>
                <div class="card-row">
                    <span>Ultrasonic:</span>
                    <span class="badge" id="ultrasonic-status" style="background-color: #197500;">ONLINE</span>
                </div>
                <div class="card-row">
                    <span>GPS:</span>
                    <span class="badge" id="gps-status" style="background-color: #197500;">ONLINE</span>
                </div>
                <div class="card-row">
                    <span>Gyroscope:</span>
                    <span class="badge" id="gyro-status" style="background-color: #197500;">ONLINE</span>
                </div>
                <div class="card-row">
                    <span>WiFi:</span>
                    <span class="badge" id="wifi-status" style="background-color: #197500;">CONNECTED</span>
                </div>
            </div>

            <!-- Combined Trashcan Height Display & Input Card -->
            <div class="card height-card">
                <div class="height-header">
                    <span>Current Height:</span>
                    <span id="current-height">0 cm</span>
                </div>
                <input type="number" class="small-input" id="height-input" placeholder="Enter new height in cm...">
            </div>
        </div>

        <!-- Right Column -->
        <div class="right-column">
            <div class="location-header">Location</div>
            <div class="map-container">
                <iframe src="about:blank" id="location"></iframe>
            </div>
        </div>
    </div>

</body>

<script>
    updateLocation();
    function updateLocation() {
        fetch('/data')
            .then(response => response.json())
            .then(data => {
                if (data.hasGPS) {
                    document.getElementById('location').src = data.mapUrl;
                } else {
                    document.getElementById('location').src = '';
                }
            });
    }
    setInterval(updateLocation, 10000);

    function updateData() {
        fetch('/data')
            .then(response => response.json())
            .then(data => {
                document.getElementById('percentage').innerText = data.percentage;
                document.getElementById('progress-bar').style.width = data.percentage;
                document.getElementById('current-height').innerText = (data.height || 0) + ' cm';
                document.getElementById('distance').innerText = (data.distance !== undefined ? data.distance : 0) + ' cm';

                // Update Tilt Angle & Lid Position Badge
                if (data.tiltAngle !== undefined) {
                    document.getElementById('gyro-angle').innerText = data.tiltAngle.toFixed(1) + '°';
                    const lidBadge = document.getElementById('lid-status-badge');
                    if (data.tiltAngle > 45) {
                        lidBadge.innerText = 'OPEN';
                        lidBadge.style.backgroundColor = '#a9ac00';
                    } else {
                        lidBadge.innerText = 'CLOSED';
                        lidBadge.style.backgroundColor = '#197500';
                    }
                }

                // System Health Status Logic
                updateBadge('esp32-status', data.esp32Status, 'ONLINE', 'OFFLINE');
                updateBadge('ultrasonic-status', data.ultrasonicStatus, 'ONLINE', 'OFFLINE');
                updateBadge('gps-status', data.hasGPS ? true : data.gpsStatus, 'ONLINE', 'OFFLINE');
                updateBadge('gyro-status', data.gyroStatus !== undefined ? data.gyroStatus : true, 'ONLINE', 'OFFLINE');
                updateBadge('wifi-status', data.wifiStatus !== undefined ? data.wifiStatus : true, 'CONNECTED', 'DISCONNECTED');

                // Collection Status Logic
                const collectionBadge = document.getElementById('collection-status');
                if (data.collectionStatus) {
                    collectionBadge.innerText = data.collectionStatus.toUpperCase();
                } else {
                    const numericFill = parseInt(data.percentage) || 0;
                    if (numericFill >= 80) {
                        collectionBadge.innerText = 'PICKUP NEEDED';
                        collectionBadge.style.backgroundColor = '#ac0000';
                    } else if (numericFill >= 50) {
                        collectionBadge.innerText = 'FILLING UP';
                        collectionBadge.style.backgroundColor = '#a9ac00';
                    } else {
                        collectionBadge.innerText = 'NORMAL';
                        collectionBadge.style.backgroundColor = '#197500';
                    }
                }

                switch (data.ledColor) {
                    case 0:
                        document.getElementById('led').style.backgroundColor = '#197500';
                        document.getElementById('led').innerText = 'GREEN';
                        break;
                    case 1:
                        document.getElementById('led').style.backgroundColor = '#a9ac00';
                        document.getElementById('led').innerText = 'YELLOW';
                        break;
                    case 2:
                        document.getElementById('led').style.backgroundColor = '#ac0000';
                        document.getElementById('led').innerText = 'RED';
                        break;
                    case 3:
                        document.getElementById('led').style.backgroundColor = '#0060ac';
                        document.getElementById('led').innerText = 'SENSOR ERROR';
                        break;
                    case 4:
                        document.getElementById('led').style.backgroundColor = '#800080';
                        document.getElementById('led').innerText = 'FINDING WIFI';
                        break;
                    case 5: 
                        document.getElementById('led').style.backgroundColor = '#0000ff';
                        document.getElementById('led').innerText = 'LID OPEN (FLASHING)';
                        break;
                    default:
                        document.getElementById('led').style.backgroundColor = '#1e293b';
                        document.getElementById('led').innerText = 'OFF';
                }
            });
    }

    function updateBadge(id, isHealthy, activeText, inactiveText) {
        const badge = document.getElementById(id);
        const active = isHealthy === true || isHealthy === 1 || isHealthy === 'ONLINE' || isHealthy === 'CONNECTED';
        
        badge.innerText = active ? activeText : inactiveText;
        badge.style.backgroundColor = active ? '#197500' : '#ac0000';
    }

    setInterval(updateData, 250);

    const heightInput = document.getElementById('height-input');

    heightInput.addEventListener('keypress', function (event) {
        if (event.key === 'Enter') {
            const newHeight = parseFloat(heightInput.value);

            if (!isNaN(newHeight) && newHeight > 0) {
                sendHeightToServer(newHeight);
                heightInput.value = ''; // Clear input box after sending
            }
        }
    });

    function sendHeightToServer(height) {
        fetch('/set-height', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ height: height })
        })
            .then(response => response.json())
            .then(data => {
                console.log('Server updated height:', data);
            })
            .catch(err => console.error('Failed to update height:', err));
    }
</script>

</html>
)rawliteral";