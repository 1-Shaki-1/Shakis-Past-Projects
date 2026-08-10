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
            overflow: hidden;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: flex-start;
            padding-top: 2vh;
            box-sizing: border-box;
        }

        /* Main Title */
        .main-title {
            font-size: 3.2vw;
            font-weight: bold;
            background-color: #1e293b;
            padding: 1.8vh 3vw;
            border-radius: 1.8vw;
            border: 0.35vw solid #334155;
            text-align: center;
            margin-bottom: 1.5vh;
            box-shadow: 0 0.4vh 0.8vh rgba(0, 0, 0, 0.3);
            letter-spacing: -0.05vw;
        }

        /* Subtitle Banner */
        .subtitle {
            font-size: 1.5vw;
            background-color: #1e293b;
            padding: 1.2vh 2.5vw;
            border-radius: 1.5vw;
            border: 0.35vw solid #334155;
            text-align: center;
            margin-bottom: 2.5vh;
            color: #f8fafc;
        }

        /* Dashboard Grid Layout */
        .dashboard-layout {
            display: flex;
            gap: 2vw;
            width: 88vw;
            height: 64vh;
            justify-content: center;
        }

        /* Left Column Cards */
        .left-column {
            display: flex;
            flex-direction: column;
            gap: 1vh;
            width: 36vw;
            height: 100%;
        }

        .card {
            background-color: #1e293b;
            border: 0.35vw solid #334155;
            border-radius: 1.5vw;
            padding: 0 1.8vw;
            font-size: 1.4vw;

            display: flex;
            align-items: center;
            justify-content: space-between;
            flex: 1;
            box-shadow: 0 0.4vh 0.8vh rgba(0, 0, 0, 0.2);
            color: #ffffff;
        }

        /* Specialized Layout for Fill Card */
        .fill-card {
            flex-direction: column;
            justify-content: center;
            align-items: stretch;
            gap: 0.8vh;
            padding: 0.8vh 1.8vw;
        }

        .fill-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            width: 100%;
        }

        /* Specialized Layout for Height Input Card */
        .height-card {
            flex-direction: column;
            justify-content: center;
            align-items: stretch;
            gap: 0.6vh;
            padding: 0.8vh 1.8vw;
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
            border: 0.2vw solid #334155;
            border-radius: 0.8vw;
            color: #ffffff;
            padding: 0.5vh 0.8vw;
            font-size: 1vw;
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
            height: 2vh;
            background-color: #0f172a;
            border-radius: 1.5vw;
            border: 0.2vw solid #334155;
            overflow: hidden;
            box-shadow: inset 0 0.2vh 0.4vh rgba(0, 0, 0, 0.5);
        }

        .progress-bar {
            height: 100%;
            width: 100%;
            /* Default width */
            background-color: #0080ff;
            /* Blue color for the fill level */
            border-radius: 1.3vw;
            transition: width 0.4s ease-out, background-color 0.4s ease-out;
        }

        /* LED Badge Pill */
        .badge {
            background-color: #117000;
            color: #ffffff;
            padding: 0.4vh 1.2vw;
            border-radius: 1vw;
            font-size: 1.2vw;
            letter-spacing: 0.1vw;
        }

        /* Right Column (Location Card) */
        .right-column {
            width: 50vw;
            height: 100%;
            background-color: #1e293b;
            border: 0.35vw solid #334155;
            border-radius: 1.8vw;
            display: flex;
            flex-direction: column;
            overflow: hidden;
            box-shadow: 0 0.4vh 0.8vh rgba(0, 0, 0, 0.2);
            padding: 1.2vh;
            box-sizing: border-box;
            gap: 1vh;
        }

        .location-header {
            font-size: 1.8vw;
            font-weight: bold;
            text-align: center;
            padding: 1.5vh;
            border: 0.35vw solid #334155;
            border-radius: 1.2vw;
            background-color: #1e293b;
        }

        .map-container {
            flex-grow: 1;
            background-color: #0f172a;
            border: 0.35vw solid #334155;
            border-radius: 1.2vw;
            display: flex;
            align-items: center;
            justify-content: center;
            overflow: hidden;
        }

        .map-container iframe {
            width: 100%;
            height: 100%;
            border: none;
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

            <!-- GPS -->
            <div class="card">
                GPS: <span id="gps">Unavailable</span>
            </div>

            <!-- LED Status (Text box removed) -->
            <div class="card">
                LED Color: <span class="badge" id="led">GREEN</span>
            </div>

            <!-- Current Trashcan Height Display -->
            <div class="card">
                Current Trashcan Height: <span id="current-height">0 cm</span>
            </div>

            <!-- Enter Trashcan Height Input -->
            <div class="card height-card">
                <div class="height-header">
                    Enter trashcan height
                </div>
                <input type="number" class="small-input" id="height-input" placeholder="Height in cm...">
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
                document.getElementById('gps').innerText = data.hasGPS ? 'YES' : 'NO';
                document.getElementById('current-height').innerText = (data.height || 0) + ' cm';

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
                    default:
                        document.getElementById('led').style.backgroundColor = '#1e293b';
                        document.getElementById('led').innerText = 'OFF';
                }
            });
    }
    setInterval(updateData, 250);


    const heightInput = document.getElementById('height-input');

    heightInput.addEventListener('keypress', function (event) {
        if (event.key === 'Enter') {
            const newHeight = parseFloat(heightInput.value);

            if (!isNaN(newHeight) && newHeight > 0) {
                sendHeightToServer(newHeight);
                heightInput.value = ' '; // Clear input box after sending
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