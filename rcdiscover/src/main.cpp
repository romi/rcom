#include <map>
#include "SerialPortConfigurationGenerator.h"
#include "SerialPortDiscover.h"
#include "SerialPortIdentification.h"

std::string dev = "/dev";

/* Since the names that controllers return don't match the JSON name in config files,
 * we need to map the device info that is returned when a device is queried to json
 * key in the config file. */
const std::map<std::string, std::string> device_to_json_key =
        {
                { "Grbl", "grbl" },
                { "BrushMotorController","brush_motors" },
                { "RomiControlPanel",  "control_panel" }
        };
int main() {

    SerialPortDiscover serialPortDiscover(device_to_json_key);
    SerialPortIdentification serialPortIdentification(serialPortDiscover);
    SerialPortConfigurationGenerator serialPortConfigurationGenerator;

    auto devices = serialPortIdentification.ListFilesOfType(dev, std::string("ACM"));

    auto connectedDevices = serialPortIdentification.ConnectedDevices(devices);

    std::string configuration = serialPortConfigurationGenerator.CreateConfiguration(connectedDevices);

    std::string port_configuration_file("serial_port_cfg.json");
    serialPortConfigurationGenerator.SaveConfiguration(port_configuration_file, configuration);

    return 0;
}
