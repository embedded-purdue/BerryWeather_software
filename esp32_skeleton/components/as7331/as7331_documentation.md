**BerryWeather AS7331 Implementation**

**Data sheet:**
Most of the information that was used to implement the sensor into this application was obtained from the data sheet, which can be found at: https://www.mouser.com/catalog/specsheets/amsOsram_AS7331_DS001047_1-00.pdf

**Files:**
In this application, there are two files that define how the microcontroller communicates with the AS7331 sensor. The header file includes two structs in which data are stored. The AS7331 struct handles the port and the device address to allow the microcontroller to communicate with the sensor; it also contains the raw light readings. The AS7331_Light struct contains the converted UV readings in which the weather station is interested.

**How information is shared:**
The ESP32 microcontroller used in our application communicates with the AS7331 through the I2C protocol. This is done by reading from and writing to the sensor's registers to initiate actions and read relevant data. The I2C address for the sensor itself is 0x74 because both A0 and A1 pins are tied to GND. There are three main types of registers to be aware of:
1. Operational state register (OSR): This register allows reading and control of sensor's operational state. It can be in either configuration mode or measurement mode.
2. Configuration Registers (CREG): These registers allow configuration of the sensor's parameters.
3. Measurement Registers (MRES): These registers allow the microcontroller to read the sensor's data, including UV readings.

**Initialization:**
1. Establish I2C connection. This ensures reliable communication between the microcontroller and the sensor.
2. Reset the sensor by writing to the OSR. This clears the sensor state to ensure consistent writing and reading.
3. Enter configuration mode and modify configuration registers. This involves writing to CREG1-3 to configure clock frequency, integration time, and gain.
    * Clock frequency: How frequently the internal clock cycles.
    * Integration time: Duration that photons accumulate before conversion. The driver programs CREG1 to request a 128 ms window, which improves signal-to-noise while staying below saturation in expected light conditions. Shorter windows are available if overflow becomes a concern, although a minimum of 64ms is needed to retrieve 2 byte measurements from measurement registers.
    * Gain: Measure of how sensitive the sensor should be to changes in light level. A gain of 2x was used for this application to limit overflow because the sensor would be subject to a high degree of light variability.
4. Enter measurement mode.

**Light Reading:**
1. Trigger a measurement by writing to the OSR.
2. Read measurement registers with a burst starting from MRES1. Each channel returns a 16-bit little-endian value (LSB then MSB), so a 6-byte buffer captures UVA, UVB, and UVC in a single transaction. These raw counts are stored in the AS7331 struct.
    * The UVC channel is clamped to 0 because longer-wavelength light leaks into the UVC channel and leads to inaccurate readings. Clamping the raw value to 0 leads to more accurate readings and calculations.
3. Obtain responsivity values from each UV channel. This factor is how we convert from raw readings to sensible UV data.
    * Typical responsivity values are given in the data sheet, but these readings are based on a 64ms integration time and a 1x gain. Since responsivity scales linearly, these responsivities must be scaled accordingly by the ratio between the used gain and integration time values, and those given by the data sheet. The driver reads the OUTCONV register to get the actual conversion time before applying the scaling.
    * These responsivity values are given in counts/(µW/cm²). By dividing the number of raw counts obtained from the measurement registers, we obtain irradiance measurements in µW/cm² (Data sheet equation 2: $E = \frac{MRES}{R}$).
4. Convert raw readings to physical values using responsivity values.

**UV Index Conversion:**
1. Determine erythema weighting function:

![Weights for different wavelengths](/components/as7331/erythema_weighting_function.png "https://gml.noaa.gov/grad/neubrew/docs/publications/McKenzie_2003.pdf")

2. Find the weights for the different wavelength channels
    * Channel A: 360nm → 0.000501
    * Channel B: 300nm → 0.648630
    * Channel C: 260nm → 1.000000
3. Multiply the weight by the irradiance values for each channel and sum them up to get the UV Index.

The initialization function and light reading function are then called in the main function with both instances of AS7331 and AS7331_Light structs as parameters. Their data can then be accessed through these structs.