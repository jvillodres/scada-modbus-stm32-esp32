# ------------------------------------------------- Includes -----------------------------------------------------------
import network
import uasyncio as asyncio
from machine import Pin
import json

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

# Preload html
html_content = ""

# Led for keep alive monitoring
led = Pin(2, Pin.OUT)

# --------------------------------------------- Public functions -------------------------------------------------------
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

# ------------------------------------------------- Asynchronous functions ----------------------------------------------
async def handle_client(reader, writer):
    """ Handle client requests on dashboard
    :param reader: Read request
    :param writer: Write response
    :return: None
    """
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

async def main():
    """ Main function for server deploy
    :return: None
    """
    load_html()
    init_ap()
    server = await asyncio.start_server(handle_client, "0.0.0.0", 80)
    print("HTTP Server running at ", AP_IP)
    async with server:
        await server.wait_closed()


# -------------------------------------------- Application entry point -------------------------------------------------
asyncio.run(main())     # Start server
