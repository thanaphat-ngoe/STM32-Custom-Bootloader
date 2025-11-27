import serial # pyright: ignore[reportMissingModuleSource]\
import time
from array import array
from enum import Enum

serial = serial.Serial(port='/dev/tty.usbmodem1203', baudrate=115200, timeout=0)

SEGMENT_BUFFER_LENGTH = 8

SEGMENT_DATA_SIZE = 32 # Up to 32 Bytes
SEGMENT_TYPE_SIZE = 1 # 1 Byte
SEGMENT_LENGTH_SIZE = 1 # 1 Byte
SEGMENT_CRC_SIZE = 1 # 1 Byte
SEGMENT_LENGTH = (SEGMENT_DATA_SIZE + SEGMENT_LENGTH_SIZE + SEGMENT_CRC_SIZE + SEGMENT_TYPE_SIZE) # Up to 35 Byte

# Define the states
class TL_STATE_T(Enum):
    TL_State_Segment_Data_Size = 1
    TL_State_Segment_Type = 2
    TL_State_Data = 3
    TL_State_Segment_CRC = 4

test_segment = array('B', [0xFF] * 35)

while(True):
    test_segment[0] = 0x00
    test_segment[1] = 0x00

    for i in range(35):
        segment_to_send = test_segment[i].to_bytes(1)
        serial.write(segment_to_send)
        segment_to_send_convert = segment_to_send[0]
        print(f"test_segment[{i}] sent = 0x{segment_to_send_convert:02x}")
    print("\n")

    time.sleep(1)

    segment_data_size = serial.read(1)
    segment_type = serial.read(1)
    segment_data = []
    for i in range(32):
        segment_data.append(serial.read(1))
    segment_crc = serial.read(1)
    
    segment_length_converted = segment_data_size[0]
    segment_type_converted = segment_type[0]
    segment_crc_converted = segment_crc[0]

    print(f"Segment Length = 0x{segment_length_converted:02x}")
    print(f"Segment Type = 0x{segment_type_converted:02x}")
    for i in range(32):
        print(f"Data[{i}] = 0x{segment_data[i][0]:02x}")
    print(f"Segment CRC = {segment_crc_converted}")
    print("\n")

    time.sleep(1)
