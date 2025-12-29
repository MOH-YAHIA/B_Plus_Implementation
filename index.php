<!DOCTYPE html>
<html>
<head>
    <title>B+ Tree</title>
</head>
<body>
    <h1>B+ Tree Operations</h1>
    
    <h2>Insert Number</h2>
    <form id="insertForm">
        <input type="number" id="insertNumber" placeholder="Enter number" required>
        <button type="submit">Insert</button>
    </form>
    <p id="insertResult"></p>
    
    <hr>
    
    <h2>Search Number</h2>
    <form id="searchForm">
        <input type="number" id="searchNumber" placeholder="Enter number" required>
        <button type="submit">Search</button>
    </form>
    <p id="searchResult"></p>
    
    <hr>
    
    <h2>Display Tree</h2>
    <button onclick="displayTree()">Display Tree Structure</button>
    <pre id="treeOutput"></pre>

    <script>
        document.getElementById('insertForm').onsubmit = function(e) {
            e.preventDefault();
            const number = document.getElementById('insertNumber').value;
            
            fetch('server.php?action=insert', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({number: number})
            })
            .then(response => response.json())
            .then(data => {
                document.getElementById('insertResult').textContent = data.message;
            })
            .catch(error => {
                document.getElementById('insertResult').textContent = 'Error: ' + error;
            });
        };
        
        document.getElementById('searchForm').onsubmit = function(e) {
            e.preventDefault();
            const number = document.getElementById('searchNumber').value;
            
            fetch('server.php?action=search', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({number: number})
            })
            .then(response => response.json())
            .then(data => {
                document.getElementById('searchResult').textContent = data.message;
            })
            .catch(error => {
                document.getElementById('searchResult').textContent = 'Error: ' + error;
            });
        };
        
        function displayTree() {
            fetch('server.php?action=display')
            .then(response => response.json())
            .then(data => {
                document.getElementById('treeOutput').textContent = data.output;
            })
            .catch(error => {
                document.getElementById('treeOutput').textContent = 'Error: ' + error;
            });
        }
    </script>
</body>
</html>
