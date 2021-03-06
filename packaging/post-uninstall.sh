#!/bin/bash
if [ -z "$2" ]; then
	rm -rf /etc/init.d/tissuestack &>> /tmp/uninstall.log
	cd /etc/init.d; update-rc.d -f tissuestack remove &>> /tmp/uninstall.log
	a2dissite tissuestack.conf &>> /tmp/uninstall.log
	a2ensite 000-default &>> /tmp/uninstall.log
	/etc/init.d/apache2 restart &>> /tmp/uninstall.log
fi
exit 0
