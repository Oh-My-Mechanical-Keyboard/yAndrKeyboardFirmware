import hid
import time

def tobytes(data):
    return [0] + data

dev = hid.device()
dev.close()
vid = 0xAA96
pid = 0xAA32

# for df in hid.enumerate():
#     if df['vendor_id'] == vid and df['product_id'] == pid and df['interface_number'] == 1:
#         keyboard = df
for df in hid.enumerate():
    if df['vendor_id'] == vid and df['product_id'] == pid and df['usage_page'] == 65280:
        keyboard = df

if keyboard == None:
    print("Keyboard was not found.")
    exit(1)
print("Keyboard found!")
print(keyboard['manufacturer_string'] + "-" + keyboard['product_string'])
dev.open_path(keyboard['path'])

print('================CLEAR=====================')


rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
time.sleep(0.1)

print('================CLEAR=====================')
print("")

print('================STAR CFG=====================')
dev.write(tobytes([0xff, 0x55, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
print('================STAR CFG=====================')
print("")

print('================GET INFO=====================')
dev.write(tobytes([0xff, 0x00, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
base_add0 = []
if len(rx_res)>0:
    base_add0 = [rx_res[1], rx_res[2], rx_res[3], rx_res[4]]

dev.write(tobytes([0xff, 0x02, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
base_add1 = []
if len(rx_res)>0:
    base_add1 = [rx_res[1], rx_res[2], rx_res[3], rx_res[4]]

dev.write(tobytes([0xff, 0x04, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
perfix  = []
if len(rx_res)>0:
    perfix = [rx_res[1], rx_res[2], rx_res[3], rx_res[4], rx_res[5], rx_res[6], rx_res[7], rx_res[8]]

dev.write(tobytes([0xff, 0x06, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
channel = 0xff
if len(rx_res) > 0:
    channel = rx_res[1]


print("--------------------")
if len(base_add0) > 0 and len(base_add1) > 0 and len(perfix) > 0 and channel != 0xff:
    print("KB INFO")
    print("base_addr0: ", base_add0)
    print("base_addr1: ", base_add1)
    print("perfix: ", perfix)
    print("channel: ", channel)
print("--------------------")

print('================GET INFO=====================')
print("")



print('================SET INFO=====================')
print('>>>SET Base Addr0')
dev.write(tobytes([0xff, 0x01, 4, 3, 2, 1, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)

dev.write(tobytes([0xff, 0x00, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
base_add0 = []
if len(rx_res)>0:
    base_add0 = [rx_res[1], rx_res[2], rx_res[3], rx_res[4]]


print('>>>SET Channel')
dev.write(tobytes([0xff, 0x07, 0x22, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)

dev.write(tobytes([0xff, 0x06, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
if len(rx_res)>0:
    print("Set new Channel: {}".format(rx_res[1]))
else:
    print("Set Channel  Fail")

print('================SET INFO=====================')


print('================END CFG=====================')
dev.write(tobytes([0xff, 0x66, 0xfe]))
time.sleep(0.1)
rx_res = dev.read(64, 10)
print("dev.read(64, 10) :",rx_res)
print('================END CFG=====================')
print("")