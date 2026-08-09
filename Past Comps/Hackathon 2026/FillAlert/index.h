const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <title>Smart Trash Can</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 10px;
            background-color: #0f172a;
            color: #ffffff;
        }

        h1 {
            color: #f8fafc;
            font-size: 100px;
            background-color: #1e293b;
            padding: 20px;
            border-radius: 10px;
            width: 50vw;
            margin: 0 auto;
            border: 10px solid #334155;
            margin-bottom: 50px;
        }

        #header {
            justify-content: center;
            top:auto;    
            font-size: 40px;
            color: #f8fafc;
            background-color: #0f172a;
            padding: 20px;
            border-radius: 10px;
            width: 85%;
            height: 5%;
            bottom:auto;
            margin-top: 0px;
            margin-bottom: 10px;

            border: 10px solid #334155;
        }


        .text {
            display: flex;
            align-items: center;
            justify-content: center;
            top:auto;    
            font-size: 40px;
            color: #f8fafc;
            background-color: #0f172a;
            padding: 20px;
            border-radius: 10px;
            width: 85%;
            height: 100%;
            bottom:auto;
           margin-top: 10px;
           margin-bottom: 10px;
            border: 10px solid #334155;
        }
        .container {
            display: flex;
            flex-direction: column;
            align-items: flex-start;
            margin: 10px;
            flex-wrap: nowrap;
            width: 20vw;
            height: 50vh;
            padding: 20px;
            border-radius: 10px;
            background-color: #1e293b;
            border: 10px solid #334155;
            
        }

        .outer-container {
            display: flex;
            justify-content: center;
            align-items: center;
            flex-wrap: wrap;
            
            
        }
        
        .container iframe {
            width: 100%;
            height: 75%;
            border: 0;
            border-radius: 10px;
            margin-top: 20px;
        }
        
    </style>
</head>

<body>
    <h1>Smart Trash Can Dashboard</h1>
        <div class="outer-container">
            <div class="container">
                <h2 class="text" id="header">Fill Level</h2>
                
                <p class="text"> <span id="percentage">0</span>%</p>
            </div>
            <div class="container">
                <h2 class="text" id="header">Has GPS?</h2>
                
                <p class="text" id="gps">NO</p>
            </div>
            <div class="container">
                <h2 class="text" id="header">LED Colour</h2>
                
                <p class="text" id="led">OFF</p>
            </div>
            <div class="container">
                <h2 class="text" id="header">Location</h2>
                
                <iframe src="https://www.google.com/maps?q=43.808805,-79.185833&output=embed" width="100%" height="65%" style="border:0;" allowfullscreen="" loading="lazy" id="location"> </iframe>
            </div>
            
        </div>
    
        

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
        setInterval(updateLocation, 60000);

        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('percentage').innerText = data.percentage;
                    document.getElementById('gps').innerText = data.hasGPS ? 'YES' : 'NO';
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
                            document.getElementById('led').style.backgroundColor = '#1e293b'; // Default background color
                            document.getElementById('led').innerText = 'OFF';
                    }
                    
                    
                });
        }
        setInterval(updateData, 250);

    </script> 
</body>

</html>
)rawliteral";