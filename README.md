# TacMesh_Controller
Arduino programming for the tacmesh controller module

This code provides the function to set ESP to sleep, depending on the mode of operation that was set by the user.
The medium of transferring data is using MQTT protocol, in sending battery voltages and receiving updates on the latest settings.
The controller will turn on and off TacMesh Radio, depending on the mode and the specific time defined by the user.
The controller having an RTC, will compare the current time with the time that was pre-set to turn on and off
