import time
import nmap
import subprocess
import sys

# curl -v 192.168.43.177:8032 --data-binary @- < esp_hello.bin

file_name = 'esp_hello.bin'

print("Scanning for connected devices...")

nm = nmap.PortScanner()
data = nm.scan(hosts="192.168.43.1/24", arguments="-sP")
active_devices = [ip for ip, result in data['scan'].items()
                  if result['status']['state'] == 'up']

for ip in active_devices:
    subprocess.Popen(['curl', '-v', ip + ':8032',
                      '--connect-timeout', '5', '--data-binary', '@' + file_name])
