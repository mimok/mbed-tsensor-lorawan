import time
import ttn
import OpenSSL
import base64

app_id = "YOUR_APP_ID"
access_key = "YOUR_ACCES_KEY"

with open("certificate.pem", 'r') as f:
  pubkey = OpenSSL.crypto.load_certificate(OpenSSL.crypto.FILETYPE_PEM, f.read())

def uplink_callback(msg, client):
  print("Received uplink from ", msg.dev_id)
  packet = base64.b64decode(msg.payload_raw)
  temp = packet[10] + packet[11]/256
  data = packet[:12+16+18+12]
  signature = packet[12+16+18+12:]
  print("temperature: " + str(temp))

  try:
    OpenSSL.crypto.verify(pubkey, signature, data, 'sha512')
  except Exception:
    print("invalid signature")
  else:
    print("valid signature")
  print()

handler = ttn.HandlerClient(app_id, access_key)

# using mqtt client
mqtt_client = handler.data()
mqtt_client.set_uplink_callback(uplink_callback)
mqtt_client.connect()
time.sleep(60)
mqtt_client.close()

# using application manager client
app_client =  handler.application()
my_app = app_client.get()
print(my_app)
my_devices = app_client.devices()
print(my_devices)
