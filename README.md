
![logo](https://static.creatordev.io/logo-md-s.svg)

# The Weather Station - Lumpy 

The Weather Station project, codenamed - **Lumpy**, allows you to create your DIY
weather station, using [Creator Ci40](https://docs.creatordev.io/ci40/) and the
[Click sensors](http://www.mikroe.com/click/). The system uses the 
[Creator IoT Framework](https://docs.creatordev.io/deviceserver/guides/iot-framework/), 
composed by AwaLWM2M and the 
[Device Server](https://docs.creatordev.io/deviceserver/guides/iot-framework/#device-server), 
providing connectivity for the sensors.

---

## Environment for Weather Station project  
The complete IoT Environment is built using the following components:
* IoT Framework
* Ci40 Application - allows the usage of clicks in mikroBUS sockets
* Contiki based applications - build for 6LoWPAN clicker platform:
  *  [Temperature Sensor](https://github.com/CreatorKit/temperature-sensor)
* Mobile Application - presents weather measurements  

## Application Dependencies

The Weather Station uses [Awa LwM2M](https://github.com/FlowM2M/AwaLWM2M), an 
implementation of OMA Lightweight M2M protocol. Awa provides a secure and standard 
compliant device management solution, without the need for an intimate knowledge of 
M2M protocols.  
The MikroE Clicks (sensors) use
[LetMeCreate](https://github.com/francois-berder/LetMeCreate), an open source library 
design to speed up the development with Ci40.


### How to Install Dependencies on Ci40

AwaLWM2M and LetMeCreate have a package ready to install on OpenWRT. For this, 
on your Ci40 console execute:

```bash
opkg update 
opkg install awalwm2m
opkg install letmecreate 
```

## How to Install the Application

To install the Weather Station, you can use the prebuild package (quick and easy),
or you can build from the source code, if you plan to edit source (advanced users).

### Install the Pre-built Package

This quick process consists in transfering the .ipk package file onto Ci40.
The latest release can be found in the releases page on this repository. You
can use **WGET** to download the file directly to Ci40.

Then, on the same directory where the .ipk package is located, complete the
installation by executing:

```bash
opkg install package_name
```

### Building From Source

This process fits for the most developers who want to edit and build from the 
source code, confortable setting the building environment for OpenWRT applications. 

It's assumed that you have build envriroment for Ci40 openWrt described 
[here](https://github.com/CreatorKit/build) and this is located in folder `work`. 
So structure inside will be:

    work/
      build/  
      constrained-os/  
      dist/
      packages/

Clone this repository into `work/packages`, after this operation structure will look:

    work/
      build/  
      constrained-os/  
      dist/
      packages/
        weather-station-gateway

Now copy folder from `packages/weather-station-gateway/feeds` into `work/dist/openwrt-ckt-feeds`.
Then execute commands:

    cd work/dist/openwrt
    ./scripts/feeds update
    ./scripts/feeds update weather-station-gateway
    ./scripts/feeds install weather-station-gateway
    make menuconfig

In menuconfig please press `/` and type `weather-station-gateway` one occurrence 
will appear. Mark it with `<*>` and do save of config.
In terminal type `make` to build openwrt image with this application.
After uploading to Ci40 edit file located in `/etc/init.d/weather_station_initd` 
and put proper switch arguments related to clicks which you put into mikroBus port.

---

## Setup For Execution

The Weather Station implements an AwaLWM2M Client to communicate the with the 
Creator Device Server. Bellow, you can find the steps to start the project on
Ci40.

**Note:** Verify if you have installed the application and its dependencies, 
described in the previous steps.  

1. First of all go to 
[**Creator Developer Console**](http://console.creatordev.io/), create an account 
and create a certificate. Certificates are used to establish a secure connection,
between the AwaLWM2M Client (Ci40) and the Device Server. Then, transfer your
**certificate.crt** into Ci40.  

2. Then execute the AwaLWM2M daemon. This daemon creates the AwaLWM2M client and 
establishes the connection against the device server. Execute on Ci40 console: 

```bash
 awa_clientd --bootstrap coaps://deviceserver.creatordev.io:15684 -s -c /root/certificate.crt --endPointName "weather_station" -o /usr/bin/weather-station/object_definitions.xml -d
```
**Note:** As the client is running as a daemon on background, any log will be 
displayed on the console. However is possible to verify the process running on 
background.

3. Finaly, connect a [thermo3 click] (http://www.mikroe.com/click/thermo3/) in
Ci40 MikroBUS 1 and execute the application.

```bash
./weatherStation -1 thermo3 -s 5 -v 5
```

If you check the [developer console](http://console.creatordev.io/), on the devices 
section is displayed a new "device" named - **weather_station**. If you explore
the objects, on **3303** its seen the temperature value measured by the sensor.

## Supported Clicks

From wide range of [MikroE clicks](http://www.mikroe.com/index.php?url=store/click/) 
in this project you can use:

| Click                                                   | WeatherStation Argument Name |
|-------------------------------------------------------- | ---------------------------- | 
| [Air Quality](http://www.mikroe.com/click/air-quality/) | air                          |
| [Carbon Monoxide](http://www.mikroe.com/click/co/)      | co                           |
| [Thermo3](http://www.mikroe.com/click/thermo3/)         | thermo3                      |
| [Thunder](http://www.mikroe.com/click/thunder/)         | thunder                      |
| [Weather](http://www.mikroe.com/click/weather/)         | weather                      |


## Help

To see all the options available, execute:

```bash
./weatherStation -h
```

This option will print the following table, on your Ci40 console:

| Switch        | Description |
|---------------|----------|
|-1, --click1   | Type of click installed in microBUS slot 1 (default:none)|
|-2, --click2   | Type of click installed in microBUS slot 2 (default:none)|
|-s, --sleep    | Delay between measurements in seconds. (default: 60s)|
|-v, --logLevel | Debug level from 1 to 5 (default:info): fatal(1), error(2), warning(3), info(4), debug(5) and max(>5)|
|-h, --help     | prints help|

On section [Supported Clicks](#supported-clicks) you can find also the arguments,
to use with the command. Example:

```bash
./weatherStation -1 [argument_name] -2 [argument_name]
```

If you have any problems installing or utilising the weather station, 
please look into our [Creator Forum](https://forum.creatordev.io). 

Otherwise, If you have come across a nasty bug or would like to suggest new 
features to be added then please go ahead and open an issue or a pull request 
against this repo.

## License

Please see the [license](LICENSE) file for a detailed explanation

## Contributing

Any Bug fixes and/or feature enhancements are welcome. Please see the 
[contributing](CONTRIBUTING.md) file for a detailed explanation.

----
