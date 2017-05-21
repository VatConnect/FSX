Initial GUI for Flight Sim World which is pretty much the FSX GUI but with SimConnect.dll converted over to load dynamically (wrapper CDSWSimConnect class defined in SimConnectFSW.h). There is no SDK yet but reports are their SimConnect.dll is little changed from FSX.

DOES NOT COMPILE YET! Compile errors are all related to DX9 stuff that needs to be converted to DX11.

Also note the files in Include and Shared are just copied into the GUI-FSW directory because I couldn't get VS2015 to include those directories. Until then any changes in them have to be copied over.

