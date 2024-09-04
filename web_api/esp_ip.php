<?php
header('Content-Type: application/json');

// Файл для хранения данных
$dataFile = 'device_data.json';

// Получаем данные из GET-параметров
$ip = isset($_GET['ip']) ? $_GET['ip'] : null;
$ssid = isset($_GET['ssid']) ? $_GET['ssid'] : null;
$signal_strength = isset($_GET['signal_strength']) ? $_GET['signal_strength'] : null;
$firmware_version = isset($_GET['firmware_version']) ? $_GET['firmware_version'] : null;
$free_heap = isset($_GET['free_heap']) ? $_GET['free_heap'] : null;
$chip_id = isset($_GET['chip_id']) ? $_GET['chip_id'] : null;
$sdk_version = isset($_GET['sdk_version']) ? $_GET['sdk_version'] : null;
$cpu_freq = isset($_GET['cpu_freq']) ? $_GET['cpu_freq'] : null;
$flash_size = isset($_GET['flash_size']) ? $_GET['flash_size'] : null;
$flash_real_size = isset($_GET['flash_real_size']) ? $_GET['flash_real_size'] : null;
$flash_speed = isset($_GET['flash_speed']) ? $_GET['flash_speed'] : null;
$status = isset($_GET['status']) ? $_GET['status'] : null;

$response = ['status' => 'error', 'message' => 'Missing parameters'];

// Проверяем, что все необходимые параметры присутствуют
if ($ip && $ssid && $signal_strength && $firmware_version && $free_heap && $chip_id &&
    $sdk_version && $cpu_freq && $flash_size && $flash_real_size && $flash_speed && $status) {

    // Если файл существует, загружаем его содержимое
    if (file_exists($dataFile)) {
        $jsonData = file_get_contents($dataFile);
        $dataArray = json_decode($jsonData, true);
    } else {
        $dataArray = [];
    }

    // Обновляем или добавляем запись в массив данных
    $dataArray[$chip_id] = [
        'ip' => $ip,
        'ssid' => $ssid,
        'signal_strength' => $signal_strength,
        'firmware_version' => $firmware_version,
        'free_heap' => $free_heap,
        'sdk_version' => $sdk_version,
        'cpu_freq' => $cpu_freq,
        'flash_size' => $flash_size,
        'flash_real_size' => $flash_real_size,
        'flash_speed' => $flash_speed,
        'status' => $status
    ];

    // Записываем обновленный массив обратно в файл
    $jsonData = json_encode($dataArray, JSON_PRETTY_PRINT);
    file_put_contents($dataFile, $jsonData);

    // Успешный ответ
    $response = ['status' => 'success', 'message' => 'Data received successfully.'];
}

// Отправляем JSON-ответ
echo json_encode($response);
?>
