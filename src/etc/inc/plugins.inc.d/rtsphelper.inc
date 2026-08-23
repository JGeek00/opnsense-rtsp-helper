<?php

function rtsphelper_enabled()
{
    global $config;

    return isset($config['installedpackages']['rtsphelper']['config'][0]['enable']);
}

function rtsphelper_firewall($fw)
{
    if (!rtsphelper_enabled()) {
        return;
    }

    $fw->registerAnchor('rtsphelper', 'rdr');
    $fw->registerAnchor('rtsphelper', 'fw');
}

function rtsphelper_services()
{
    $services = array();

    if (!rtsphelper_enabled()) {
        return $services;
    }

    $pconfig = array();
    $pconfig['name'] = 'rtsphelper';
    $pconfig['description'] = gettext('RTSP Helper');
    $pconfig['php']['restart'] = array('rtsphelper_stop', 'rtsphelper_start');
    $pconfig['php']['start'] = array('rtsphelper_start');
    $pconfig['php']['stop'] = array('rtsphelper_stop');
    $pconfig['pidfile'] = '/var/run/rtsphelper.pid';
    $services[] = $pconfig;

    return $services;
}

function rtsphelper_start()
{
    if (!rtsphelper_enabled()) {
        return;
    }

    if (isvalidpid('/var/run/rtsphelper.pid')) {
        return;
    }

    shell_exec('/usr/sbin/daemon -f /usr/local/bin/python3 /usr/local/opnsense/scripts/net/rtsphelper/rtsphelper.py');
}

function rtsphelper_stop()
{
    killbypid('/var/run/rtsphelper.pid', 'TERM', true);
    shell_exec('/sbin/pfctl -a rtsphelper -Fr >/dev/null 2>&1');
    shell_exec('/sbin/pfctl -a rtsphelper -Fn >/dev/null 2>&1');
}

function rtsphelper_configure()
{
    return array('bootup' => array('rtsphelper_configure_do'));
}

function rtsphelper_permuser_list()
{
    $ret = array();
    $count = 3;

    for ($i = 1; $i <= $count; $i++) {
        $ret[$i] = "permuser{$i}";
    }

    return $ret;
}

function rtsphelper_forward_list()
{
    $ret = array();
    $count = 10;

    for ($i = 1; $i <= $count; $i++) {
        $ret[$i] = "forward{$i}";
    }

    return $ret;
}

function rtsphelper_expand_cidr($forward_entry)
{
    $result = array();

    // Parse format: IP[/CIDR]:Port
    $parts = explode(':', $forward_entry);
    if (count($parts) != 2) {
        return $result;
    }

    $ip_part = $parts[0];
    $port = $parts[1];

    // Check if CIDR is present
    if (strpos($ip_part, '/') === false) {
        // Single IP, return as-is
        $result[] = $forward_entry;
        return $result;
    }

    list($network, $cidr) = explode('/', $ip_part);
    $cidr = intval($cidr);

    // Calculate number of hosts
    $host_count = pow(2, 32 - $cidr);

    // Limit expansion to reasonable sizes to avoid memory issues
    // For /24 = 256 hosts, /16 = 65536 hosts (might be too much)
    if ($host_count > 65536) {
        // Don't expand very large networks, log warning
        log_error("RTSP Helper: CIDR /{$cidr} is too large to expand ({$host_count} hosts). Skipping.");
        return $result;
    }

    // Convert IP to long
    $ip_long = ip2long($network);
    if ($ip_long === false) {
        return $result;
    }

    // Calculate network address
    $mask = -1 << (32 - $cidr);
    $network_long = $ip_long & $mask;

    // Generate all IPs in range
    for ($i = 0; $i < $host_count; $i++) {
        $current_ip_long = $network_long + $i;
        $current_ip = long2ip($current_ip_long);
        $result[] = "{$current_ip}:{$port}";
    }

    return $result;
}

function rtsphelper_configure_do($verbose = false)
{
    global $config;

    rtsphelper_stop();

    if (!rtsphelper_enabled()) {
        return;
    }

    if ($verbose) {
        echo 'Starting RTSP Helper...';
        flush();
    }

    $rtsphelper_config = $config['installedpackages']['rtsphelper']['config'][0];

    $ext_ifname = get_real_interface($rtsphelper_config['ext_iface']);
    if ($ext_ifname == $rtsphelper_config['ext_iface']) {
        if ($verbose) {
            echo "failed.\n";
        }
        return;
    }

    $config_text = "ext_ifname={$ext_ifname}\n";

    $ifaces_active = '';

    /* RTSP Helper access restrictions */
    foreach (rtsphelper_permuser_list() as $permuser) {
        if (!empty($rtsphelper_config[$permuser])) {
            $config_text .= "allow={$rtsphelper_config[$permuser]}\n";
        }
    }

    foreach (rtsphelper_forward_list() as $forward) {
        if (!empty($rtsphelper_config[$forward])) {
            $forward_entry = $rtsphelper_config[$forward];
            $expanded_forwards = rtsphelper_expand_cidr($forward_entry);
            foreach ($expanded_forwards as $expanded) {
                $config_text .= "forward={$expanded}\n";
            }
        }
    }
    /* write out the configuration */
    file_put_contents('/var/etc/rtsphelper.conf', $config_text);
    rtsphelper_start();

    if ($verbose) {
        echo "done.\n";
    }
}
