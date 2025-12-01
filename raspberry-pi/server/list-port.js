import { SerialPort } from "serialport";

console.log("🔍 Scanning for available serial ports...\n");

SerialPort.list().then(ports => {
  if (ports.length === 0) {
    console.log("❌ No serial ports found!");
    console.log("\nMake sure your device is:");
    console.log("  - Connected via USB");
    console.log("  - Powered on");
    console.log("  - Drivers installed (check Device Manager)\n");
    return;
  }

  console.log(`✅ Found ${ports.length} port(s):\n`);
  
  ports.forEach((port, i) => {
    console.log(`${i + 1}. ${port.path}`);
    if (port.manufacturer) console.log(`   Manufacturer: ${port.manufacturer}`);
    if (port.serialNumber) console.log(`   Serial Number: ${port.serialNumber}`);
    if (port.pnpId) console.log(`   PnP ID: ${port.pnpId}`);
    console.log();
  });

  console.log("💡 Update SERIAL_PORT in windows-server.js to match your sensor's port");
  console.log("   Example: const SERIAL_PORT = \"COM3\";\n");
}).catch(err => {
  console.error("Error listing ports:", err);
});