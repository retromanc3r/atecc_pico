#include "atecc_pico.h"

// Reads response safely with error checking
int i2c_read_blocking_safe(uint8_t *response, size_t expected_length) {
    int res = i2c_read_blocking(I2C_PORT, I2C_ADDR, response, expected_length, false);
    if (res != expected_length) {
        printf("❌ ERROR: I2C read failed (expected %zu, got %d)\n", expected_length, res);
        return -1;
    }
    return res;
}

// Send a wake-up sequence to the ATECC608
bool wake_device() {    
    uint8_t data = 0x00;
    uint8_t wake_response[4];

    // Send wakeup sequence with proper delays
    i2c_write_blocking(I2C_PORT, 0x00, &data, 1, false);
    sleep_ms(3);

    int res = i2c_read_blocking(I2C_PORT, I2C_ADDR, wake_response, sizeof(wake_response), false);

    printf("🛰️ **Wake-up Response:** ");
    for (int i = 0; i < 4; i++) {
        printf("%02X ", wake_response[i]);
    }
    printf("\n");

    // Check if wake-up response matches expected value
    if (res > 0 && wake_response[0] == 0x04 && wake_response[1] == 0x11 &&
        wake_response[2] == 0x33 && wake_response[3] == 0x43) {
        printf("✅ Wake-up successful!\n");
        return true;
    } else {
        printf("❌ ERROR: Wake-up failed! Unexpected response.\n");
        return false;
    }
}

// Wrapper function around i2c_write_blocking to send a command to the ATECC608
bool send_command(uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t data_len) {
    uint8_t command[7 + data_len];
    command[0] = 0x07 + data_len;
    command[1] = opcode;
    command[2] = param1;
    command[3] = param2 & 0xFF;
    command[4] = (param2 >> 8) & 0xFF;

    if (data_len > 0) {
        memcpy(&command[5], data, data_len);
    }

    calc_crc16_ccitt(5 + data_len, command, &command[5 + data_len]);

    uint8_t full_command[8 + data_len];
    full_command[0] = 0x03;
    memcpy(&full_command[1], command, sizeof(command));

    return i2c_write_blocking(I2C_PORT, I2C_ADDR, full_command, sizeof(full_command), false) >= 0;
}

// Wrapper function to read response from the ATECC608
bool get_response(uint8_t *buffer, size_t length, bool full_response) {
    uint8_t response[7];
    size_t read_length = full_response ? 7 : length + 1;
    
    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, read_length, false) != (int)read_length) {
        printf("❌ ERROR: Failed to read response from ATECC608\n");
        return false;
    }
    
    memcpy(buffer, &response[1], length);
    return true;
}

// Function to send an idle command to ATECC608
bool send_idle_command() {
    uint8_t idle_cmd = 0x02; // Idle command
    if (!i2c_write_blocking(I2C_PORT, I2C_ADDR, &idle_cmd, 1, false)) {
        printf("❌ ERROR: Failed to send idle command!\n");
        return false;
    }
    return true;
}

// Read the serial number of the ATECC608
bool read_serial_number() {
    uint8_t serial[9];
    uint8_t last_response[2];

    send_command(OP_READ, 0x00, 0x0000, NULL, 0);
    sleep_ms(5);
    if (!get_response(&serial[0], 4, true)) return false;

    send_command(OP_READ, 0x00, 0x0002, NULL, 0);
    sleep_ms(5);
    if (!get_response(&serial[4], 5, true)) return false;

    send_command(OP_READ, 0x00, 0x0003, NULL, 0);
    sleep_ms(5);
    if (!get_response(last_response, 2, false)) return false;
    serial[8] = last_response[0];

    printf("🆔 Serial Number: ");
    for (int i = 0; i < 9; i++) {
        printf("%02X", serial[i]);
    }
    printf("\n");

    return true;
}

// Map a random number to a specific range
uint64_t map_random_to_range(uint8_t *random_bytes, uint64_t min, uint64_t max) {
    uint64_t random_value = 0;
    for (int i = 0; i < 8; i++) {  // Using only 64 bits out of the 256-bit random number
        random_value = (random_value << 8) | random_bytes[i];
    }
    return min + (random_value % (max - min + 1));
}

// Generate a random number in a specific range using the ATECC608A
void generate_random_number_in_range(uint64_t min, uint64_t max) {
    uint8_t response[35];

    send_command(OP_RANDOM, 0x00, 0x0000, NULL, 0);
    sleep_ms(23);

    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, sizeof(response), false) != (int)sizeof(response)) {
        printf("❌ ERROR: Failed to read random number response\n");
        return;
    }

    if (response[0] != 0x23) {
        printf("❌ ERROR: Invalid response length byte!\n");
        return;
    }

    uint64_t mapped_value = map_random_to_range(&response[1], min, max);

    printf("🎲 Random Number (Mapped to Range %llu-%llu): %llu\n", min, max, mapped_value);
}

// Generate a random 256-bit value using the ATECC608A
bool generate_random_value(uint8_t length) {
    uint8_t response[35];

    send_command(OP_RANDOM, 0x00, 0x0000, NULL, 0);
    sleep_ms(23);

    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, sizeof(response), false) != (int)sizeof(response)) {
        printf("❌ ERROR: Failed to read random number response\n");
        return false;
    }

    if (response[0] != 0x23) {
        printf("❌ ERROR: Invalid response length byte!\n");
        return false;
    }

    printf("🎲 Random Value (HEX): ");
    for (int i = 4; i < length; i++) {
        printf("%02X ", response[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
}

// Generate a SHA-256 hash using the ATECC608
bool compute_sha256_hash(const char *message) {
    size_t message_len = strlen(message);
    size_t offset = 0;
    uint8_t response[35];

    // Step 1: Start SHA computation
    if (!send_command(OP_SHA, 0x00, 0x0000, NULL, 0)) {
        printf("❌ ERROR: SHA Start command failed!\n");
        return false;
    }
    sleep_ms(5);

    // Step 2: Process full 64-byte blocks (SHA Update)
    while (message_len - offset >= 64) {
        if (!send_command(OP_SHA, 0x01, 0x0000, (const uint8_t *)&message[offset], 64)) {
            printf("❌ ERROR: SHA Update command failed!\n");
            return false;
        }
        offset += 64;
        sleep_ms(5);
    }

    // Step 3: Process the final block (SHA End)
    if (!send_command(OP_SHA, 0x02, message_len - offset, (const uint8_t *)&message[offset], message_len - offset)) {
        printf("❌ ERROR: SHA End command failed!\n");
        return false;
    }
    sleep_ms(5);

    // Step 4: Read the response
    int res = i2c_read_blocking_safe(response, sizeof(response));
    if (res < 0 || response[0] != 0x23) {
        printf("❌ ERROR: Failed to retrieve SHA-256 digest!\n");
        return false;
    }

    // Step 5: Validate CRC
    if (!validate_crc(response, sizeof(response))) {
        printf("❌ ERROR: CRC check failed for response!\n");
        return false;
    }

    // Step 6: Print Hash
    printf("🔢 SHA-256: ");
    for (int i = 1; i <= 32; i++) {
        printf("%02X", response[i]);
    }
    printf("\n");

    return true;
}

// Read the configuration of a specific slot
bool read_slot_config(uint8_t slot) {
    uint8_t response[4];
    printf("🔎 Checking Slot %d Configuration...\n", slot);

    if (!send_command(OP_READ, 0x00, slot, NULL, 0)) {
        printf("❌ ERROR: Failed to send slot config read command!\n");
        return false;
    }
    sleep_ms(20);

    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, 4, false) != 4) {
        printf("❌ ERROR: Failed to read slot configuration!\n");
        return false;
    }

    printf("🔎 Slot %d Config Data: %02X %02X %02X %02X\n", slot, response[0], response[1], response[2], response[3]);
    return true;
}

// Read the configuration zone of the ATECC608
bool read_config_zone() {
    uint8_t config_data[128] = {0};  // Full 128-byte config zone
    uint8_t response[5];  // Read buffer (5 bytes to include index byte)
    int config_index = 0;  // Tracks where to store valid bytes

    printf("🔎 Reading Configuration Data...\n");

    for (uint8_t i = 0; i < TOTAL_READS; i++) {
        if (!send_command(OP_READ, 0x00, i, NULL, 0)) {
            printf("❌ ERROR: Failed to send read command for index %d!\n", i);
            return false;
        }
        sleep_ms(20);

        if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, 5, false) != 5) {  // Read 5 bytes
            printf("❌ ERROR: Failed to read configuration for index %d!\n", i);
            return false;
        }

        // Store only the last 4 bytes, skipping the index byte (0x07)
        if (config_index + 4 <= 128) {
            config_data[config_index]     = response[1];
            config_data[config_index + 1] = response[2];
            config_data[config_index + 2] = response[3];
            config_data[config_index + 3] = response[4];
            config_index += 4;  // Move forward by exactly 4 bytes
        }
    }

    // Print formatted config data (16 bytes per row)
    for (int i = 0; i < 128; i++) {
        printf("%02X ", config_data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    return true;
}

// Check the lock status of the ATECC608
bool check_lock_status() {
    printf("🔍 Checking ATECC608A Lock Status...\n");

    uint8_t response[5];  // Read 5 bytes to ensure full data capture
    uint8_t expected_address = 0x15;  // Correct address for lock bytes

    // 🔹 Send read command for lock status at word address 0x15
    if (!send_command(OP_READ, 0x00, expected_address, NULL, 0)) {
        printf("❌ ERROR: Failed to send lock status read command!\n");
        return false;
    }

    sleep_ms(23);
    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, 5, false) != 5) { 
        printf("❌ ERROR: Failed to read lock status response!\n");
        return false;
    }

    // 🔍 Print raw response for debugging
    printf("🔐 Raw Lock Status Response: %02X %02X %02X %02X %02X\n",
           response[0], response[1], response[2], response[3], response[4]);

    // Extract correct lock bytes from response[3] and response[4]
    uint8_t lock_config = response[3];  // Byte 0x15 (Config Lock)
    uint8_t lock_value = response[4];   // Byte 0x16 (Data Lock)

    printf("🔒 Config Lock Status: %02X\n", lock_config);
    printf("🔒 Data Lock Status: %02X\n", lock_value);

    // 🔐 Determine Lock Status
    if (lock_config == 0x00 && lock_value == 0x00) {
        printf("🔒 Chip is **FULLY LOCKED** (Config & Data).\n");
        return true;
    } 
    else if (lock_config == 0x55 && lock_value == 0x55) {
        printf("🔓 Chip is **UNLOCKED**.\n");
        return true;
    } 
    else if (lock_config == 0x00 && lock_value == 0x55) {
        printf("⚠️ Chip is **PARTIALLY LOCKED** (Config Locked, Data Open).\n");
        return true;
    } 
    else {
        printf("❓ **UNKNOWN LOCK STATE**: Unexpected lock values, possible read error.\n");
        return false;
    }
}

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    printf("📡 Initializing ATECC608...\n");

    // Send a wake-up sequence
    if (!wake_device()) {
        printf("❌ ERROR: Failed to wake up ATECC608\n");
        return 1;
    }
    
    // Read the serial number
    if (!read_serial_number()) {
        printf("❌ ERROR: Failed to read Serial Number\n");
        return 1;
    }

    // Generate a random number in a specific range
    generate_random_number_in_range(100, 65535);

    // Compute a SHA-256 hash 
    
    if (!compute_sha256_hash("COLD WAR")) {
        printf(" ERROR: Failed to compute a SHA-256 hash\n");
        return 1;
    }
    
    // Read the configuration of a specific slot
    if (!read_slot_config(0x00)) {
        printf("❌ ERROR: Failed to read slot configuration\n");
        return 1;
    }

    // Generate a random value of specific length
    if (!generate_random_value(16)) {
        printf("❌ ERROR: Failed to generate random value\n");
        return 1;
    }

    // Read the configuration data of all slots
    if (!read_config_zone()) {
        printf("❌ ERROR: Failed to read configuration data\n");
        return 1;
    }

    // Read the configuration data of all slots and check the lock status
    if (!check_lock_status()) {
        printf("❌ ERROR: Failed to check lock status\n");
        return 1;
    }

    printf("🎉 ATECC608 Test Complete!\n");

    return 0;
}
