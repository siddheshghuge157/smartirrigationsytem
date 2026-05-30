#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

/*
 * SMART IRRIGATION SYSTEM - WEB INTERFACE
 * HTML, CSS, and JavaScript for web dashboard
 */

const char* getWebPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Irrigation System</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        .status-bar {
            display: flex;
            justify-content: space-around;
            background: #f8f9fa;
            padding: 20px;
            flex-wrap: wrap;
        }
        .status-item {
            text-align: center;
            padding: 15px;
            background: white;
            border-radius: 10px;
            margin: 5px;
            min-width: 150px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .status-value {
            font-size: 2em;
            font-weight: bold;
            color: #667eea;
            margin: 10px 0;
        }
        .status-label {
            color: #666;
            font-size: 0.9em;
        }
        .content {
            padding: 30px;
        }
        .section {
            margin-bottom: 30px;
            background: #f8f9fa;
            padding: 20px;
            border-radius: 10px;
        }
        .section h2 {
            color: #1e3c72;
            margin-bottom: 20px;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
        }
        .control-group {
            margin: 15px 0;
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 10px;
            background: white;
            border-radius: 5px;
        }
        label {
            font-weight: 500;
            color: #333;
        }
        input[type="number"], input[type="text"], input[type="password"], select {
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 1em;
            width: 150px;
        }
        input[type="checkbox"] {
            width: 20px;
            height: 20px;
            cursor: pointer;
        }
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 25px;
            cursor: pointer;
            font-size: 1em;
            font-weight: bold;
            transition: transform 0.2s;
        }
        button:hover {
            transform: scale(1.05);
        }
        button:active {
            transform: scale(0.95);
        }
        .btn-danger {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }
        .btn-success {
            background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
        }
        .pump-status {
            text-align: center;
            padding: 30px;
            font-size: 1.5em;
        }
        .pump-on {
            color: #00f2fe;
            animation: pulse 1s infinite;
        }
        .pump-off {
            color: #999;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .schedule-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 15px;
        }
        .schedule-card {
            background: white;
            padding: 15px;
            border-radius: 10px;
            border: 2px solid #ddd;
        }
        .schedule-card.active {
            border-color: #667eea;
            background: #f0f4ff;
        }
        .fertilizer-alert {
            background: #fff3cd;
            border: 2px solid #ffc107;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            font-size: 1.2em;
        }
        .fertilizer-alert.warning {
            background: #f8d7da;
            border-color: #dc3545;
        }
        @media (max-width: 768px) {
            .status-bar { flex-direction: column; }
            .control-group { flex-direction: column; align-items: flex-start; }
            input[type="number"], input[type="text"], select { width: 100%; margin-top: 10px; }
        }
    </style>
</head>
<body>
    <!-- PASTE THE REST OF THE HTML FROM PREVIOUS COMPLETE CODE -->
    <!-- (I'll continue in next file to save space) -->
</body>
</html>
)rawliteral";
}

#endif // WEB_INTERFACE_H