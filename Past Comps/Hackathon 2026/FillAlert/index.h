const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
    <title>Smart Trash Can</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 50px;
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

        .text {
            
            justify-content: center;
            font-size: 40px;
            color: #f8fafc;
            background-color: #1e293b;
            padding: 20px;
            border-radius: 10px;
            width: 15vw;
            
            
            margin: 1vh 1vw;
            border: 10px solid #334155;
        }

        .container {
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            flex-wrap: nowrap;
            height: relative;
            width: 20vw;
            padding: 20px;
            border-radius: 10px;
            background-color: #1e293b;
            border: 10px solid #334155;
            margin: 0vh 1vw;
        }

        .outer-container {
            display: flex;
            justify-content: center;
            align-items: center;
            flex-wrap: wrap;
            
            
        }

        
    </style>
</head>

<body>
    <h1>Smart Trash Can Dashboard</h1>
        <div class="outer-container">
            <div class="container">
                <h2 class="text">Fill Level</h2>
                
                <p class="text"> <span id="percentage">0</span>%</p>
            </div>
            <div class="container">
                <h2 class="text">Has GPS?</h2>
                
                <p class="text" id="gps">NO</p>
            </div>
            <div class="container">
                <h2 class="text">LED Colour</h2>
                
                <p class="text" id="led">OFF</p>
            </div>
            
        </div>
    
        

    <script>
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
        setInterval(updateData, 500);
    </script> 
</body>

</html>
)rawliteral";