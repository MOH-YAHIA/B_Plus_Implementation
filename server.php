<?php
// server.php - Main server file

// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Set JSON header for AJAX requests
header('Content-Type: application/json');

// Path to your exe file
$exePath = "bplustree_imp.exe";

// Function to run the exe with input
function runExe($exePath, $input) {
    // Create descriptors for stdin, stdout, stderr
    $descriptors = array(
        0 => array("pipe", "r"),  // stdin
        1 => array("pipe", "w"),  // stdout
        2 => array("pipe", "w")   // stderr
    );
    
    // Start the process
    $process = proc_open($exePath, $descriptors, $pipes);
    
    if (!is_resource($process)) {
        return array('success' => false, 'message' => 'Failed to start exe');
    }
    
    // Write input to stdin
    fwrite($pipes[0], $input);
    fclose($pipes[0]);
    
    // Read output from stdout
    $output = stream_get_contents($pipes[1]);
    fclose($pipes[1]);
    
    // Read errors from stderr
    $errors = stream_get_contents($pipes[2]);
    fclose($pipes[2]);
    
    // Close the process
    $return_value = proc_close($process);
    
    return array(
        'success' => true,
        'output' => $output,
        'errors' => $errors,
        'return_code' => $return_value
    );
}

// Get the request method
$method = $_SERVER['REQUEST_METHOD'];

// Handle different operations based on URL
$action = isset($_GET['action']) ? $_GET['action'] : '';

if ($method === 'POST' && $action === 'insert') {
    // INSERT operation
    $data = json_decode(file_get_contents('php://input'), true);
    $number = isset($data['number']) ? $data['number'] : null;
    
    if ($number === null) {
        echo json_encode(array('success' => false, 'message' => 'No number provided'));
        exit;
    }
    
    // Create input: 1 (insert), number, 5 (exit)
    $input = "1\n" . $number . "\n5\n";
    
    $result = runExe($exePath, $input);
    
    if ($result['success']) {
        echo json_encode(array(
            'success' => true,
            'message' => "Successfully inserted $number",
            'output' => $result['output']
        ));
    } else {
        echo json_encode($result);
    }
    
} elseif ($method === 'POST' && $action === 'search') {
    // SEARCH operation
    $data = json_decode(file_get_contents('php://input'), true);
    $number = isset($data['number']) ? $data['number'] : null;
    
    if ($number === null) {
        echo json_encode(array('success' => false, 'message' => 'No number provided'));
        exit;
    }
    
    // Create input: 2 (search), number, 5 (exit)
    $input = "2\n" . $number . "\n5\n";
    
    $result = runExe($exePath, $input);
    
    if ($result['success']) {
        // Check if number was found
        $found = strpos($result['output'], 'FOUND') !== false && 
                 strpos($result['output'], 'NOT FOUND') === false;
        
        echo json_encode(array(
            'success' => true,
            'found' => $found,
            'message' => "Number $number " . ($found ? "FOUND" : "NOT FOUND"),
            'output' => $result['output']
        ));
    } else {
        echo json_encode($result);
    }
    
} elseif ($method === 'GET' && $action === 'display') {
    // DISPLAY operation
    // Create input: 3 (display), 5 (exit)
    $input = "3\n5\n";
    
    $result = runExe($exePath, $input);
    
    if ($result['success']) {
        echo json_encode(array(
            'success' => true,
            'output' => $result['output']
        ));
    } else {
        echo json_encode($result);
    }
    
} else {
    echo json_encode(array('success' => false, 'message' => 'Invalid request'));
}
?>
