#!/bin/sh

SOURCE="https://raw.githubusercontent.com/JGeek00/opnsense-rtsp-helper/master/src"

curl -o /usr/local/etc/inc/plugins.inc.d/rtsphelper.inc $SOURCE/etc/inc/plugins.inc.d/rtsphelper.inc

mkdir -p /usr/local/opnsense/mvc/app/models/OPNsense/RTSPHelper/ACL/
curl -o /usr/local/opnsense/mvc/app/models/OPNsense/RTSPHelper/ACL/ACL.xml $SOURCE/opnsense/mvc/app/models/Net/RTSPHelper/ACL/ACL.xml

mkdir -p /usr/local/opnsense/mvc/app/models/OPNsense/RTSPHelper/Menu/
curl -o /usr/local/opnsense/mvc/app/models/OPNsense/RTSPHelper/Menu/Menu.xml $SOURCE/opnsense/mvc/app/models/Net/RTSPHelper/Menu/Menu.xml

mkdir -p /usr/local/opnsense/scripts/net/rtsphelper/
curl -o /usr/local/opnsense/scripts/net/rtsphelper/rtsphelper.py $SOURCE/opnsense/scripts/net/rtsphelper/rtsphelper.py

curl -o /usr/local/www/services_rtsphelper.php $SOURCE/www/services_rtsphelper.php
curl -o /usr/local/www/status_rtsphelper.php $SOURCE/www/status_rtsphelper.php
