// This sample code decode from NMEA to decimal degree format

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Function to convert NMEA latitude/longitude to decimal degrees
double nmea_to_decimal(const char *nmea, char direction) {
    // Extract degrees and minutes
    double raw_value = atof(nmea); // Convert string to double
    int degrees = (int)(raw_value / 100); // Extract the degree part
    double minutes = raw_value - (degrees * 100); // Extract the minute part

    // Convert to decimal degrees
    double decimal = degrees + (minutes / 60.0);

    // Adjust for direction
    if (direction == 'S' || direction == 'W') {
        decimal = -decimal;
    }

    return decimal;
}

int main() {
    // NMEA sentence
    char nmea[] = "$GPGLL,2059.36851,N,10556.66232,E,040954.00,A,A*66";

    // Variables to store extracted values
    char latitude[20], lat_dir, longitude[20], lon_dir;

    // Tokenize the sentence using commas as delimiters
    char *token = strtok(nmea, ",");

    // Check if the first token is "$GPGLL"
    if (strcmp(token, "$GPGLL") == 0) {
        // Extract latitude
        token = strtok(NULL, ",");
        strcpy(latitude, token);

        // Extract latitude direction (N/S)
        token = strtok(NULL, ",");
        lat_dir = token[0];

        // Extract longitude
        token = strtok(NULL, ",");
        strcpy(longitude, token);

        // Extract longitude direction (E/W)
        token = strtok(NULL, ",");
        lon_dir = token[0];

        // Convert to decimal degrees
        double lat_decimal = nmea_to_decimal(latitude, lat_dir);
        double lon_decimal = nmea_to_decimal(longitude, lon_dir);

        // Print results
        printf("Latitude: %.8f\n", lat_decimal);
        printf("Longitude: %.8f\n", lon_decimal);
    } else {
        printf("Invalid NMEA sentence.\n");
    }

    return 0;
}
