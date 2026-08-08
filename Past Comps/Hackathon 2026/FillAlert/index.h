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
            background-color: #121212;
            color: #ffffff;
        }

        h1 {
            color: #4CAF50;
            font-size: 100px;
        }

        .percentage {
            font-size: 40px;
            color: #ffffff;
            float: left;
            margin-left: 20px;
        }

        .percentage-container {
            display: flex;
            align-items: left;
            height: 100vh;
            padding-left: 20px;
            background-color: #1e1e1e;
        }
    </style>
</head>

<body>
    <h1>Smart Trash Can Dashboard</h1>
    <div class="percentage-container">
        <p class="percentage">Fill Level: <span id="dist">Loading...</span>%</p>
    </div>
    

    <script>
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('dist').innerText = data.percentage;
                });
        }
        setInterval(updateData, 250);
    </script> 
</body>

</html>
)rawliteral";