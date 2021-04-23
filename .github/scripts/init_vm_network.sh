#! /bin/bash

# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#
# This script install dependencies of service side of e2e tests and runs e2e test

ip link add veth0 type veth peer name veth1
ifconfig veth0 192.168.1.1 netmask 255.255.255.0 up
ifconfig veth1 up
ethtool --offload veth0 tx off
ethtool --offload veth1 tx off

sh -c "cat > /etc/dhcp/dhcpd.conf" <<EOT
subnet 192.168.1.0 netmask 255.255.255.0 {
    range 192.168.1.100 192.168.1.200;
    option routers 192.168.1.1;
    option domain-name-servers 1.1.1.1;
    option broadcast-address 192.168.1.255;
    default-lease-time 600;
    max-lease-time 7200;
}
EOT
sh -c "cat > /etc/default/isc-dhcp-server" <<EOT
INTERFACES="veth0"
EOT
systemctl restart isc-dhcp-server

sysctl net.ipv4.ip_forward=1
iptables -F
iptables -t nat -F
iptables -t nat -A POSTROUTING -s 192.168.1.0/24 -o eth0 -j MASQUERADE
iptables -A FORWARD -d 192.168.1.0/24 -o veth0 -j ACCEPT
iptables -A FORWARD -s 192.168.1.0/24 -j ACCEPT

#Debug
ip link
ip addr
iptables -t nat -L -n
cat /etc/dhcp/dhcpd.conf
cat /etc/default/isc-dhcp-server
