/* This file is part of Interactive Atlast Forth Interpreter For ESP32.
 * Copyright (C) 2021  Vojtech Fryblik <433796@mail.muni.cz>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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