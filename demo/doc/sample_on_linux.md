# Sample on Linux

## Download Programs

    ```bash
    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install -y \
        cmake \
        gcc-multilib \
        g++-multilib \
        libcmocka0:i386 \
        libpcap-dev:i386 \
        npm
    ```

## Sample Authentication and Configuration

Authentication values will be updated in the [demo_config.h](./demo/sample_azure_iot_embedded_sdk/demo_config.h) file.

For either Provisioning or IoT Hub samples, SAS key and X509 authentication are supported. Update either `democonfigDEVICE_SYMMETRIC_KEY` for symmetric key or client side certificate `democonfigCLIENT_CERTIFICATE_PEM` and `democonfigCLIENT_PRIVATE_KEY_PEM` for certificates. **Note: only one auth mechanism can be used at a time and the other unused macros should be commmented out.**

If you need help generating a cert and private key, see the below [Generating a Cert](#generating-a-cert) section.

If you would like to use Device Provisioning, update the following values:

- `democonfigENDPOINT`
- `democonfigID_SCOPE`
- `democonfigREGISTRATION_ID`

If you would like to connect straight to IoT Hub, comment out `democonfigENABLE_DPS_SAMPLE` and update the following values:

- `democonfigDEVICE_ID`
- `democonfigHOSTNAME`

## Virtual Interface

Create virtual interface with dhcp (if not present).

    ```bash
    #! /bin/bash

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
    ```

## Run sample with interface (veth1)

*Note: edit FreeRTOSConfig.h  and configNETWORK_INTERFACE_TO_USE to index of veth1. User can find index by running sample once and it prints the index of all the interfaces.*

    ```bash
    sudo ./build/sample_azure_iot_embedded_sdk
    ```

## Generating a Cert

If you need a working x509 certificate to get the samples working please see the following:

    ```bash
    openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
    openssl req -new -days 30 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=azure-freertos-device"
    openssl x509 -noout -text -in device_ec_cert.pem

    rm -f device_cert_store.pem
    cat device_ec_cert.pem device_ec_key.pem > device_cert_store.pem

    openssl x509 -noout -fingerprint -in device_ec_cert.pem | sed 's/://g'| sed 's/\(SHA1 Fingerprint=\)//g' | tee fingerprint.txt
    ```

This will output a self signed client certificate with a private key. The `fingerprint.txt` is for your convenience when adding the device in IoT Hub and it asks for a Primary and Secondary Fingerprint. This value can be used for both fields.
