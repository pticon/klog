klog - a GNU/Linux keylogger
============================

klog tries to log all of the keys pressed on the keyboard.
By default, klog starts as a daemon and you should send SIGTERM
in order to stop it.

Warning
-------
No part of klog may be used to break the law, or to cause damage of any
kind. And I'm not responsible for anything you do with it.

Usage
-----
```
klog [options] <eventid>

options:

	-h       : display this and exit

	-q       : quiet mode
	
	-v       : display version number and exit
	
	-o <log> : write log to <log> (default=stdout)
	
	-m <map> : load the map character <map> (default=./map/en_US.map)
	
	-f       : keep on foreground (eg: no daemon)

	-p <pid> : write the pid in the <pid> file (default=/var/run/klog.pid)
```

Example
-------
```
./klog -f -m ./map/fr_FR.map -o- 5
```

klog will log all of the keys pressed on "/dev/input/event5" as a
french keyboard on the standard output. Moreover, it will run in
the foreground without daemonize.

License
-------
klog is licensed under the BSD 3-clause license.
