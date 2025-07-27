#include "bp_public.h"
#include "buttplugCpp/include/buttplugclient.h"

#define BP_PREFIX "[Buttplug] "

bpimport_t bi;

// connection
cvar_t* bp_address;
cvar_t* bp_port;
// in-game behavior
cvar_t* bp_treshold;
cvar_t* bp_damage;
cvar_t* bp_fight_strength;

Client* client;
bool hasDevice;
DeviceClass selectedDevice;
float cachedIntensity;

bool CheckClient()
{
	if (!client)
		bi.Printf(BP_PREFIX "No client found, run \\bp_restart, idk\n");

	return client != nullptr;
}

void callbackFn([[maybe_unused]] const mhl::Messages msg)
{
	/*
	switch (msg.messageType) {
	case mhl::MessageTypes::DeviceAdded:
		bi.Printf(BP_PREFIX "Device %s (%d) added\n",
			msg.deviceAdded.device.DeviceName.c_str(),
			msg.deviceAdded.device.DeviceIndex
		);
		break;
	case mhl::MessageTypes::DeviceRemoved:
		bi.Printf(BP_PREFIX "Device %d removed\n",
			msg.deviceRemoved.DeviceIndex
		);
		break;

	default:
		break;
	}
	*/
}

void ButtplugStart()
{
	client = new Client(bp_address->string, bp_port->integer);
	client->connect(callbackFn);
}

void ButtplugStop()
{
	if (!CheckClient())
		return;

	if (hasDevice) {
		client->stopAllDevices();
		// client->disconnect();
	}
	delete client;
}

void BP_Restart()
{
	if (!client)
		return;

	ButtplugStop();
	ButtplugStart();
}

void ButtplugScan(bool wait)
{
	if (!CheckClient())
		return;

	if (hasDevice) {
		client->stopAllDevices();
		hasDevice = false;
	}

	client->requestDeviceList();
	client->startScan();

	if (bi.Cmd_Argc() == 2 && !strcmp(bi.Cmd_Argv(1), "wait")) {
		// sleep to await scan
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}

	client->stopScan();

	auto devices = client->getDevices();

	switch (devices.size()) {
	case 0:
		bi.Printf(BP_PREFIX "No devices found! Run \\bp_scan [wait]\n");
		break;
	case 1:
		bi.Printf(BP_PREFIX "Single device found, auto-selecting it\n");
		selectedDevice = devices[0];
		hasDevice = true;
		break;
	default:
		bi.Printf(BP_PREFIX "Multiple devices found, there is a list of them:\n");
		for (const auto& device : devices) {
			bi.Printf("%d : %s\n", device.deviceID, device.displayName.c_str());
		}
		bi.Printf(BP_PREFIX "Select your device by using \\bp_scanSelect <id>\n");
	}
}

void BP_Scan()
{
	bool wait = bi.Cmd_Argc() == 2 && !strcmp(bi.Cmd_Argv(1), "wait");
	ButtplugScan(wait);
}

void BP_ScanSelect()
{
	if (bi.Cmd_Argc() != 2) {
		bi.Printf(BP_PREFIX "Usage: \\bp_scanSelect <id>.\n");
		bi.Printf(BP_PREFIX "ID you can get by using \\bp_scan\n");
		return;
	}

	if (!CheckClient())
		return;

	auto devices = client->getDevices();

	if (devices.size() < 2) {
		bi.Printf(BP_PREFIX "Are you serious? Run \\bp_scan\n");
		return;
	}

	int idx = atoi(bi.Cmd_Argv(1));

	// I don't know, is it worth to just devices[idx]
	// or enumerate all and use deviceID we initially printed?
	for (const auto& device : devices) {
		if (device.deviceID == idx) {
			bi.Printf(BP_PREFIX "%s selected!\n", device.displayName.c_str());
			selectedDevice = device;
			hasDevice = true;
			break;
		}
	}
}

void BP_Status()
{
	bi.Printf(BP_PREFIX "Client is %srunning\n", client == nullptr ? "NOT " : "");
	bi.Printf(BP_PREFIX "Device was %sselected\n", hasDevice ? "" : "NOT ");
	if (hasDevice)
		bi.Printf(BP_PREFIX "Selected device name is %s\n", selectedDevice.displayName.c_str());
}

void BP_Init()
{
	client = nullptr;
	hasDevice = false;

	// WebSocket address of Intiface, could be wss://
	bp_address = bi.Cvar_Get("bp_address", "ws://127.0.0.1", CVAR_ARCHIVE);
	// WebSocket Intiface port
	bp_port = bi.Cvar_Get("bp_port", "12345", CVAR_ARCHIVE);

	// Speed treshold when vibration starts
	bp_treshold = bi.Cvar_Get("bp_treshold", "400", CVAR_ARCHIVE);
	// Enable vibration on any kind of damage
	bp_damage = bi.Cvar_Get("bp_damage", "1", CVAR_ARCHIVE);
	// Strength of attack
	bp_fight_strength = bi.Cvar_Get("bp_fight_strength", "0.25", CVAR_ARCHIVE);

	bi.Cmd_AddCommand("bp_restart", BP_Restart);
	bi.Cmd_AddCommand("bp_scan", BP_Scan);
	bi.Cmd_AddCommand("bp_scanSelect", BP_ScanSelect);
	bi.Cmd_AddCommand("bp_status", BP_Status);

	ButtplugStart();
	ButtplugScan(false);
}

void BP_Stop()
{
	ButtplugStop();

	bi.Cmd_RemoveCommand("bp_restart");
	bi.Cmd_RemoveCommand("bp_scan");
	bi.Cmd_RemoveCommand("bp_scanSelect");
	bi.Cmd_RemoveCommand("bp_status");
}

qboolean BP_Vibrate(float intensity)
{
	if (!CheckClient())
		return qfalse;

	if (!hasDevice)
	{
		// bi.Printf(BP_PREFIX "Device wasn't selected!\n");
		return qfalse;
	}

	if (cachedIntensity == intensity)
		return qtrue;
	cachedIntensity = intensity;

	client->sendScalar(selectedDevice, intensity);
	return qtrue;
}

void BP_StopVibrate()
{
	if (!CheckClient())
		return;

	if (!hasDevice)
		return;

	if (cachedIntensity == 0.0f)
		return;
	cachedIntensity = 0.0f;

	client->stopDevice(selectedDevice);
}

static bpexport_t bexp {
	.Init = BP_Init,
	.Stop = BP_Stop,
	.Vibrate = BP_Vibrate,
	.StopVibrate = BP_StopVibrate,
};

extern "C"
Q_EXPORT bpexport_t* QDECL GetBPAPI(bpimport_t* bimp)
{
	bi = *bimp;
	return &bexp;
}
