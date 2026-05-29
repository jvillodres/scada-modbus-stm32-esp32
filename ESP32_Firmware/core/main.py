# ------------------------------------------------- Includes -----------------------------------------------------------
import network
import uasyncio as asyncio
from machine import Pin, SPI
import json
import utime

# ---------------------------------------------- Public variables ------------------------------------------------------
# Access Point configuration
AP_SSID = "SCADA_ESP32"
AP_KEY = "scada1234"
AP_IP = "192.168.4.1"

#  Registers state (copy of Modbus Slave)
registers = {
    "motor":        0,      # 41001 - Motor ON/OFF      (digital)
    "velocidad":    0,      # 41002 - Speed %           (analog)
    "brazo":        0,      # 41003 - Classifier arm    (digital)
    "presencia":    0,      # 41004 - Presence sensor   (digital)
    "temperatura":  25,     # 41005 - Temperature °C    (analog)
    "contador":     0,      # 41006 - Counter pieces    (analog)
    "alarma":       0       # 41007 - Overheating       (analog)
}

# SCADA pending data to STM32
pending = {
    "motor":        0xFF,
    "velocidad":    0xFF,
    "brazo":        0xFF
}
has_pending = False

# Preload html
html_content = ""

# Led for keep alive monitoring
led = Pin(2, Pin.OUT)

# SPI configuration
spi = SPI(1,
          baudrate=500000,
          polarity=0,
          phase=0,
          bits=8,
          firstbit=SPI.MSB,
          sck=Pin(19),
          mosi=Pin(23),
          miso=Pin(18)
          )
cs = Pin(5, Pin.OUT)

# For detecting HMI changes (Signal handled by STM32)
drdy = Pin(4, Pin.IN)

# ------------------------------------------------- Initializations ----------------------------------------------------
cs.value(1)

# ------------------------------------------------- Public functions ---------------------------------------------------
def init_ap():
    """ Initialize the access point
    :return: None
    """
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid=AP_SSID, password=AP_KEY, authmode=3)
    print("AP active: ", ap.ifconfig())

def load_html():
    """ Load the dashboard HTML file
    :return: None
    """
    global html_content
    with open("index.html", 'r') as f:
        html_content = f.read()
    print("HTML loaded: ", len(html_content), "bytes")

def crc8(data):
    """ Calculate CRC-8 for SPI communication
    :param data: Frame to calculate CRC-8
    :return: CRC calculated
    """
    crc = 0
    for byte in data:
        crc ^= byte
    return crc

# ------------------------------------------------- Asynchronous functions ----------------------------------------------
async def handle_client(reader, writer):
    """ Handle client requests on dashboard
    :param reader: Read request
    :param writer: Write response
    :return: None
    """
    global has_pending
    try:
        request = await reader.read(1024)
        request = request.decode()

        # GET /data -> return JSON registers
        if "GET /data" in request:
            body = json.dumps(registers)
            response = (
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                f"Content-length: {len(body)}\r\n"
                "\r\n" + body
            )

        # POST /set -> receives change on dashboard
        elif "POST /set" in request:
            body_start = request.find("\r\n\r\n") + 4
            body = request[body_start:]
            data = json.loads(body)

            for key in data:
                if key in registers:
                    registers[key] = data[key]
                if key in pending:
                    pending[key] = data[key]
                    has_pending = True

            response = (
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-length: 2\r\n"
                "\r\n{}"
            )

        # GET / -> serves html dashboard
        else:
            response = (
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Connection: close\r\n"
                "\r\n" + html_content
            )

        writer.write(response.encode())
        await writer.drain()

    except Exception as e:
        print("Client error: ", e)

    finally:
        writer.close()
        await writer.wait_closed()

async def spi_task():
    """
    Handles all SPI related tasks. Build a TX frame with the SCADA data and updates the SCADA with RX frame from STM32
    :return: None
    """
    global has_pending
    while True:
        # Build TX frame
        tx = bytearray(10)
        tx[0] = 0xAA
        tx[1] = pending["motor"]
        tx[2] = pending["velocidad"]
        tx[3] = pending["brazo"]
        tx[4] = 0x00
        tx[5] = 0x00
        tx[6] = 0x00
        tx[7] = 0x00
        tx[8] = 0x00
        tx[9] = crc8(tx[1:9])

        # SPI Transfer
        rx = bytearray(10)
        cs.value(0)
        utime.sleep_us(10)
        spi.write_readinto(tx, rx)
        utime.sleep_us(10)
        cs.value(1)

        # Clear pendings after transfer
        pending["motor"] = 0xFF
        pending["velocidad"] = 0xFF
        pending["brazo"] = 0xFF
        has_pending = False

        # Process RX frame from STM32
        if rx[0] == 0xBB:
            calc_crc = crc8(rx[1:9])
            if calc_crc == rx[9]:
                registers["motor"] = rx[1]
                registers["velocidad"] = rx[2]
                registers["brazo"] = rx[3]
                registers["presencia"] = rx[4]
                registers["temperatura"] = rx[5]
                registers["contador"] = (rx[6] << 8) | rx[7]
                registers["alarma"] = rx[8]

            else:
                print("CRC error RX: ", hex(calc_crc), "!=", hex(rx[9]))

        else:
            print("Invalid start byte: ", hex(rx[0]))

        await asyncio.sleep_ms(200)

async def main():
    """ Main function for server deploy
    :return: None
    """
    load_html()
    init_ap()
    asyncio.create_task(spi_task())
    server = await asyncio.start_server(handle_client, "0.0.0.0", 80)
    print("HTTP Server running at ", AP_IP)
    async with server:
        await server.wait_closed()


# -------------------------------------------- Application entry point -------------------------------------------------
asyncio.run(main())     # Start server
