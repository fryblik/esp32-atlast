/**
 * Get WLAN config JSON
 * 
 * Deserialize and load WLAN configuration from config.json.
 * Loads the first network configuration in the "networks" array.
 * Return true on success, false otherwise.
 */
bool getWlanConfigJSON();

/**
 * Get WLAN config serial
 * 
 * Fill wlanConf structure with user-provided credentials.
 * Return true if user provided credentials, false otherwise.
 */
bool getWlanConfigSerial();

/**
 * Connect to WLAN
 */
bool connectWlan();