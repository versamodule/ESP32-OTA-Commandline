# OTA Example


Yet another OTA upload. 
In menunconfig, edit the STATION_SSID & STATION_PASSPHRASE to match your router settings, then build. 
After you flash to the ESP32, using your favorite terminal program watch the boot up information to see what IP was leased
to you device. Then open a command prompt, and following this example upload your new firmware to it.

curl 192.168.0.3:8032 --data-binary @- < build/OTA_Template.bin

NOTE: Of course, changing it to fit your situation. I.E. change the IP to match yours, and after the "<" character the folder/file location. 


Check the Video out here:
https://www.youtube.com/watch?v=rfec9lrQ1E0&feature=youtu.be




