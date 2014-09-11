1) how to make
     make && make install
     make test && make test_install

2) redirtion filter settings file
     /etc/usbredir_rules.conf 

3) run testing
     usbredir_test -h     show help string
     usbredir_test -s serverip -p portlist
       case:
         usbredir_test -s 192.168.0.2 -p 5000
         usbredir_test -s 192.168.0.2 -p 5000:6000

Release v1.0
    requirement:
      libusb >=1.0.16   (libusb.info)
      usbredir >=0.6 
    
    Funcationality:
      support redirect usb device to "Qemu/KVM virtual machine"
      Dynamic hot plug&unplug unsupported.

