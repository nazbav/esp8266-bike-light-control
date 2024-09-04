@echo off
setlocal

rem Задание параметров по умолчанию
set "default_ip=*.*.*.*"
set "default_firmware_path=%USERPROFILE%\Documents\Arduino\bike_light\build\esp8266.esp8266.d1_mini\bike_light.ino.bin"

rem Проверка наличия переданных параметров
set "ip=%~1"
set "firmware_path=%~2"

rem Использование параметров по умолчанию, если не заданы
if "%ip%"=="" set "ip=%default_ip%"
if "%firmware_path%"=="" set "firmware_path=%default_firmware_path%"

rem Вывод используемых значений
echo Using IP: %ip%
echo Using firmware path: %firmware_path%

rem Активируем сервер обновлений на ESP8266
echo Activating update server...
curl -X GET "http://%ip%/startUpdate"

rem Проверка наличия файла прошивки
if not exist "%firmware_path%" (
    echo Firmware file not found: %firmware_path%
    exit /b 1
)

rem Загрузка прошивки
echo Uploading firmware...
curl -F "firmware=@%firmware_path%" "http://%ip%/update"

echo Update completed.

endlocal

pause